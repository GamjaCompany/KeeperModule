#include <Arduino.h>
#include <SimpleTimer.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <env.h>
#include <HW_config.h>
#include <Network_config.h>
#define FOR(i, b, e) for(int i = b; i < e; i++)

// 1. 네트워크 연결 되면 5초마다 2번 빠르게 점멸
// 2. WiFi 정보는 LIFFS에 저장
// 3. 주변에 있는 WiFi와 LIFFS 저장되있는 WiFi와 일치 시 자동 연결 시도
// 4. 주변에 와이파이가 없을 경우 LED 빠르게 점멸
// 5. 저장되있는 비밀번호로 5초 이상 연결 시도에도 무반응 시 연결 차단
// 6. 연결 상태에서 갑작스러운 연결 해제 시 감지 가능
// 7. LittleFS저장소를 FTP를 통해 접근 및 수정 가능
// 8. LED점멸기능을 뺴고 싶을 경우 HW_config.h에서 단순히 헤더 참조 빼면 됨

void setup() {
    Serial.begin(115200); // 시리얼 통신 초기화

    hw_init();
    env.init();
    net.init();
}

void loop() {   
    if (net.isAvailable()) {
        String recv = net.get_mqtt_recv();
        
        // 사용중인 저장소 용량 확인하는 명령어
        if (recv == "LittleFS" || recv == "lfs") {
            char msg[256]; memset(msg, '\0', 256);

            sprintf(msg, "Total: %dbyte\nUsed: %dbyte", LittleFS.totalBytes(), LittleFS.usedBytes());
            
            net.publish("status", msg);

            return;
        }
        // 현재 접속된 WiFi 및 주변 WiFi 확인하는 명령어
        if (recv == "Network" || recv == "net") {
            char tmp[128]; memset(tmp, '\0', 128);

            sprintf(tmp, "[현재 연결된 와이파이]\nSSID: %s (%ddbm)\n내부아이피: %s\n", 
                WiFi.SSID().c_str(), 
                WiFi.RSSI(), 
                WiFi.localIP().toString().c_str()
            );

            net.publish("status", tmp);
            
            // 비동기 스캔 시작
            WiFi.scanNetworks(true);

            return;
        }
    }
    
    net.run();
    #ifdef LED_HANDLER_H
    led.run();
    #endif
}