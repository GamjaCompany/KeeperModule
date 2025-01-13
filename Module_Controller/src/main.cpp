#include "env.h"  // 개인정보(와이파이 ID/PW, MQTT서버 url/ID/PW)
#include <Arduino.h>
#include <MqttHandler.h>

/** 
 * MQTT메시지 해석 함수
 * @param {String} - 토픽
 * @param {String} - 페이로드
*/
void handleRecv(String topic, String payload);

void setup() {
    Serial.begin(115200);

    setup_wifi();
}

void loop() {
    if (!client.connected()) reconnect();
    
    client.loop();
}

void handleRecv(String topic, String payload) {
    Topic_CMD cmd = getCMD_idx(topic);  // 토픽을 하드코딩한 int형 Index값으로 변환함 (switch-case문 쓸려고)

    switch(cmd) {
        case CONTROL: // Control토픽 수신 시
            { Serial.println("[Control 토픽 수신] : " + payload);
            } break;
        case AI:
            { Serial.println("[AI 토픽 수신] : " + payload);

            } break;
        case GET:  // GET토픽 수신 시
            { Serial.println("[GET 토픽 수신] : " + payload);

            } break;
        case DEBUG:  // DEBUG토픽 수신 시
            { Serial.println("[DEBUG 토픽 수신] : " + payload);

            } break;
        default:
            Serial.printf("예외 토픽 수신!!!\n");
    }
}



