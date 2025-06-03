#ifndef NETWORK_CONFIG_H_
#define NETWORK_CONFIG_H_

/* 개요: WiFi, MQTT, FTP를 관리하는 헤더 입니다 
 * --------------------------------------------
 * 1. 주변 WiFi를 스캔 후, LittleFS에 저장된 WiFi정보로 자동 접속합니다
 * 2. 저장되있는 WiFI정보에서 Password가 틀릴 시 자동으로 차단 합니다
 * 3. WiFi에 접속 성공 시 MQTT서버에 접속합니다
 * 4. WiFi에 접속 성공 시 FTP를 구축합니다
*/

#include <Arduino.h>
#include <WiFi.h>
#include <env.h>
#include <WiFiclientSecure.h>
#include <SimpleTimer.h>
#include <PubSubclient.h>
#include <SimpleFTPServer.h>
#include <queue>

#define FOR(i, b, e) for(int i = b; i < e; i++)

#define AUTH_WRONG -1
#define FAILED -1
#define MQTT_MSG_QUEUE_SIZE 4
#define MQTT_MSG_CHUNK_SIZE 512

void _callback(FtpOperation ftpOperation, unsigned int freeSpace, unsigned int totalSpace);
void _transferCallback(FtpTransferOperation ftpOperation, const char* name, unsigned int transferredSize);
void mqtt_callback(char* topic, uint8_t* payload, unsigned int length);

typedef struct Wifi_info {
    String ssid;
    String password;
    int32_t RSSI;
    String Encryption;  // 암호화 여부
} Wifi_info;

class Network_Handler {
    private:
        Wifi_info current_info;
        Wifi_info scaned_list[32];
        String mqtt_recv;
        bool is_recv;
        bool isConnected;  // WiFi객체를 써도 되지만, 명시적으로 관리하기 위해 상태변수를 생성
        bool isConnecting;  // 연결 시도 중을 명시적으로 표현하기 위해 생성
        bool isDEBUG_mode;
        int16_t wifi_cnt;
        
        SimpleTimer reScanTimer;
        SimpleTimer connectingTimer;
        SimpleTimer reconnectMQTT_Timer;
        SimpleTimer connect_timeout_Timer;

        WiFiClientSecure espclient;
        PubSubClient mqtt_client;
        
        FtpServer ftpSrv;
        
        // WiFi에 연결 or 끊겼을 때 동작시킬 외부 함수 ( 콜백 )
        std::vector<std::function<void()>> onConnect_cb_list;
        std::vector<std::function<void()>> onDisconnect_cb_list;
        
        // MQTT브로커 서버에 연결되지 않았을 때 메시지 임시 저장소
        std::queue<std::pair<String, String>> pending_msgs;
        
    public:
        Network_Handler() = default;
        Network_Handler& operator=(const Network_Handler& ref) = delete;  
        static Network_Handler& GetInstance();
        String getSSID() { return current_info.ssid; }
        
        // 와이파이가 연결되었을 때 호출 시키고 싶은 함수를 등록 ( 반환형: void, 인자: void )
        void reg_connected_callback(std::function<void()> cb_func);
        
        // 와이파이가 연결해제 됐을 때 호출 시키고 싶은 함수를 등록 ( 반환형: void, 인자: void )
        void reg_disconnected_callback(std::function<void()> cb_func);
        
        // 초기화
        void init();

        // 수신받은 데이터를 객체에 업데이트 ( 이때 상태 값도 업데이트 됨 )
        void set_mqtt_recv(String input);
        
        // 수신받은 데이터 불러오기 ( 이때 상태 값도 업데이트 됨 )
        String get_mqtt_recv();
        
        // 스캔 결과 출력 (mqtt 서버 연결 중 일시 거기에도 출력)
        void print_all_scan_results();
        
        //  SPIFFS에 저장되있는 WiFi가 주변에 있는지 검색( return: ssid_list의 인덱스 )
        String search_available_network();

        // Network 구성 ( 전제조건으로 WiFi 스캔이 완료되있어야 함 )
        bool begin_network_setup();
        
        // MQTT브로커 서버 재접속
        void reconnect();
        
        // MQTT 브로커 서버에서 수신받은 데이터가 있는지 확인
        bool isAvailable() { return is_recv; }

        // WiFi 스캔 완료인지 확인
        bool isScanComplete() { return 0 <= wifi_cnt; }
        
        // WiFi가 예기치 않게 접속 해제 됐는지 확인
        bool isUnexpectedDisconnectd() { return isConnected && !WiFi.isConnected(); }
        
        // WiFi 스캔 중 인지 확인
        bool isScanning() { return WiFi.scanComplete() == -1; }
        
        // MQTT브로커 서버 설정
        void setMQTT();

        // mqtt publish
        void publish(String topic, String msg);
        
        void publish(String topic, String *msg);
        
        // non-blocking 실행
        void run();
    
}; Network_Handler& net = Network_Handler::GetInstance();

Network_Handler& Network_Handler::GetInstance() {
    static Network_Handler instance;
    
    return instance;
}

// 초기화
void Network_Handler::init() { 
    isDEBUG_mode = true; 
    reScanTimer.setInterval(5000);
    connectingTimer.setInterval(100); 
    reconnectMQTT_Timer.setInterval(2000);
    connect_timeout_Timer.setInterval(8000);
    mqtt_client.setClient(espclient);
    mqtt_client.setCallback(nullptr);
    isConnected = false;
    isConnecting = false;
    
    // 초기화 했으니 스캔 시작
    WiFi.scanNetworks(true);
}

// 수신받은 데이터를 객체에 업데이트 ( 이때 상태 값도 업데이트 됨 )
void Network_Handler::set_mqtt_recv(String input) {
    mqtt_recv = input;
    is_recv = true;
}

// 수신받은 데이터 불러오기 ( 이때 상태 값도 업데이트 됨 )
String Network_Handler::get_mqtt_recv() {
    is_recv = false;
    
    return mqtt_recv;
}

// 스캔 결과 출력 (mqtt 서버 연결 중 일시 거기에도 출력)
void Network_Handler::print_all_scan_results() {
    char tmp[128]; memset(tmp, '\0', 64);
    String scan_log;
    
    FOR(i, 0, wifi_cnt) {
        scaned_list[i].ssid = WiFi.SSID(i);
        scaned_list[i].RSSI = WiFi.RSSI(i);
        scaned_list[i].Encryption = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "" : "[*]";
    }
    
    scan_log = "";
    FOR(i, 0, wifi_cnt) {
        memset(tmp, '\0', 64);

        // 출력할 문자열을 한 줄 씩 생성후 msg에 연결(concat)
        sprintf(
            tmp, 
            "SSID: %s%s (%ddbm)\n", 
            scaned_list[i].ssid.c_str(), scaned_list[i].Encryption.c_str(), scaned_list[i].RSSI
        );
        
        scan_log += tmp;
    }
    
    // 전처리한 정보 출력
    Serial.println(scan_log);
    
    // MQTT 브로커 연결돼 있을 시 전송
    if (mqtt_client.connected()) {
        Serial.printf("Send data size: %dbyte", scan_log.length());
        if (mqtt_client.connected()) {
            mqtt_client.beginPublish("status", scan_log.length(), false);
            mqtt_client.print(scan_log.c_str());
            mqtt_client.endPublish();
        }
    }
    
}

//  SPIFFS에 저장되있는 WiFi가 주변에 있는지 검색( return: ssid_list의 인덱스 )
String Network_Handler::search_available_network() {
    for(Wifi_info& obj : scaned_list) {      
        for(JsonPair env_wifi_iter : env.wifi_list) {
            String     env_wifi_ssid = env_wifi_iter.key().c_str();
            JsonArray  env_wifi_info = env_wifi_iter.value();
            int        env_wifi_status = env_wifi_iter.value()[(int)EnvData::STATUS];

            // SPIFFS에 저장된 WiFi가 있으며 비밀번호가 틀린 이력이 없는가
            if (env_wifi_status != AUTH_WRONG && env_wifi_ssid == obj.ssid) 
                return env_wifi_ssid;
        }
    }

    return "NULL";
}

// Network 구성 ( 전제조건으로 WiFi 스캔이 완료되있어야 함 )
bool Network_Handler::begin_network_setup() {
    // LittleFS에 저장되있는 WiFi가 주변에 있는지 검색
    String find_ssid = search_available_network();
    
    // 검색 결과 없으면 주기적으로 연결 재시도
    if (find_ssid == "NULL") {
        Serial.println("!!!! 사용가능한 와이파이 없음 !!!!");
        
        #ifdef LED_HANDLER_H 
        led.set(2000, 50, 5);
        #endif
        
        // 스캔데이터 초기화
        WiFi.scanDelete();

        return false;
    }
    
    // 여기까지 왔으면 LittleFS에 저장된 정보로 연결 시도를 할꺼임
    current_info.ssid     = find_ssid;
    current_info.password = env.wifi_list[find_ssid][(int)EnvData::PASSWORD].as<String>();

    Serial.println("Connecting to " + current_info.ssid);

    WiFi.mode(WIFI_STA);
    WiFi.begin(current_info.ssid, current_info.password);
    
    connect_timeout_Timer.reset();
    
    isConnecting = true;  // 연결 중을 명시적으로 표현
    
    #ifdef LED_HANDLER_H 
    led.set(100, NOT_USE_BLINK);
    #endif
    
    return true;
}

// MQTT브로커 서버 재접속
void Network_Handler::reconnect() {
    String mqtt_clientId = "ESP32mqtt_client-";
    mqtt_clientId += String(random(0xffff), HEX);

    if (mqtt_client.connect(mqtt_clientId.c_str(), env.mqtt.user_id, env.mqtt.user_password)) {
        Serial.println("MQTT Broker connected!!");
        
        // 만약, 연결해제 상태에서 MQTT브로커 서버로 보낼 메시지가 있었을 때
        while (!pending_msgs.empty()) {
            auto [topic, msg] = pending_msgs.front();

            Serial.println("묵혀온 메시지 전송! -> " + msg);

            publish(topic.c_str(), msg.c_str());
            pending_msgs.pop();
        }

        // 접속이 완료되면 본인의 내부아이피 주소 전송
        char tmp[32]; memset(tmp, '\0', 32);

        sprintf(tmp, "wake-up! : %s", WiFi.localIP().toString().c_str());
        publish("status", tmp);
        
        mqtt_client.subscribe("cmd");
    } else {
        Serial.print("failed, rc=");
        Serial.print(mqtt_client.state());
        Serial.println(" try again in 2 seconds");
    }
}

// MQTT브로커 서버 설정
void Network_Handler::setMQTT() {
    // MQTT연결 설정
    // env.mqtt.broker_address, env.mqtt.broker_port 이거 2개 출력
    Serial.print("Broker Address: ");
    Serial.println(env.mqtt.broker_address);
    Serial.print("Port: ");
    Serial.println(env.mqtt.broker_port);
    
    espclient.setInsecure();
    mqtt_client.setServer(env.mqtt.broker_address, env.mqtt.broker_port);
    mqtt_client.setCallback(mqtt_callback);
    reconnect();
}

void Network_Handler::publish(String topic, String msg) {
    if (mqtt_client.connected()) {
        char name_prefix[32]; memset(name_prefix, '\0', 32);
        
        // 현재 기기의 이름을 접두사로 해서 전송합니다
        sprintf(name_prefix, "[%s] ", env.getName().c_str());

        mqtt_client.beginPublish(topic.c_str(), msg.length()+strlen(name_prefix), false);
        mqtt_client.print(name_prefix);
        mqtt_client.print(msg.c_str());
        mqtt_client.endPublish();

        return;
    } 
    
    try {
        if (MQTT_MSG_QUEUE_SIZE <= pending_msgs.size())     
            throw "큐의 크기를 초과 합니다";
        if (256 < msg.length())       
            throw "보관할 메시지가 너무 깁니다";
        
        Serial.printf("[메시지 저장] Broker 서버 연결 시 전송합니다!\n");
        
        pending_msgs.push({topic, msg});
    }
    catch (const char* err) {
        Serial.printf("[메시지 드랍] 사유: %s\n", err);
    }
}

void Network_Handler::publish(String topic, String *msg) {
    if (mqtt_client.connected()) {
        char name_prefix[32]; memset(name_prefix, '\0', 32);
        
        // 현재 기기의 이름을 접두사로 해서 전송합니다
        sprintf(name_prefix, "[%s] ", env.getName().c_str());
        
        mqtt_client.beginPublish(topic.c_str(), msg->length()+strlen(name_prefix), false);
        mqtt_client.print(name_prefix);
        
        if (MQTT_MSG_CHUNK_SIZE < msg->length()) {
            Serial.printf("메시지 크기 큼!!! 분할해서 송신!!! (%d)\n", msg->length());
            
            for (size_t i = 0; i < msg->length(); i += MQTT_MSG_CHUNK_SIZE) {
                int len = std::min((size_t)MQTT_MSG_CHUNK_SIZE, msg->length() - i);
                
                int written = mqtt_client.write((const uint8_t*)(msg->c_str() + i), len);
                
                if (written != len) {
                    Serial.println("MQTT chunk write failed");
                    mqtt_client.endPublish();
                    
                    return;
                }
            }
            
            return;
        }
        
        mqtt_client.print(msg->c_str());
        mqtt_client.endPublish();

        return;
    } 
    
    try {
        if (MQTT_MSG_QUEUE_SIZE <= pending_msgs.size())     
            throw "큐의 크기를 초과 합니다";
        if (256 < msg->length())       
            throw "보관할 메시지가 너무 깁니다";
        
        Serial.printf("[메시지 저장] Broker 서버 연결 시 전송합니다!\n");
        
        pending_msgs.push({topic, *msg});
    }
    catch (const char* err) {
        Serial.printf("[메시지 드랍] 사유: %s\n", err);
    }
}
    
void Network_Handler::run() {
    wifi_cnt = WiFi.scanComplete();
    
    // 예기치 않게 접속 해제 당했을 때
    if (isUnexpectedDisconnectd()) {
        Serial.printf("[AP-OFF] %s → AP전원 꺼짐\n", current_info.ssid.c_str());
        
        #ifdef LED_HANDLER_H 
        led.set(2000, 50, 5);
        #endif
        
         // 등록한 콜백함수 실행 ( 연결해제 됐을 때 )
        for(std::function<void()> fn_ptr : onDisconnect_cb_list) {
            fn_ptr();
        }

        // 완전한 중단
        WiFi.disconnect(true, true);
        WiFi.mode(WIFI_OFF);
        espclient.stop();
        espclient = WiFiClientSecure();  // 새 인스턴스 할당 ← ★ 중요!
        espclient.setInsecure();
        
        // 명시적으로 접속 해제를 표시
        isConnected = false;
        
        current_info.ssid = "";
        current_info.password= "";
    }
    
    // 재스캔 타이머
    if (!isConnecting && reScanTimer.isReady()) {
        // 와이파이 연결이 없고, 이미 스캔 중이 아닐 때
        if (!isScanning() && !WiFi.isConnected()) {
            Serial.println("[Network_config] 스캔시작.");

            WiFi.scanDelete();  
            
            WiFi.scanNetworks(true);
            
            #ifdef LED_HANDLER_H 
            led.set(500, NOT_USE_BLINK);  // 점멸
            #endif
        }
        
        reScanTimer.reset();
    }
    
    // 비동기 스캔 완료 시
    if (isScanComplete()) {
        print_all_scan_results();
        
        if (!isConnected) begin_network_setup();
        
        // 스캔데이터 초기화
        WiFi.scanDelete();
    }
    
    // 연결 시도 중 일때
    if (isConnecting) {
        if (connectingTimer.isReady()) {
            Serial.print(".");
            
            connectingTimer.reset();
        }

        if (WiFi.isConnected()) {
            randomSeed(micros());
            
            Serial.println("");
            Serial.println("WiFi connected");
            Serial.println("IP address: ");
            Serial.println(WiFi.localIP());

            // MQTT브로커 서버 연결 시작
            setMQTT();
            
            // 등록한 콜백함수 실행 ( 연결됐을 때 )
            for(std::function<void()> fn_ptr : onConnect_cb_list) {
                fn_ptr();
            }
            
            if (LittleFS.begin(true)) {
                ftpSrv.setCallback(_callback);
                ftpSrv.setTransferCallback(_transferCallback);
                ftpSrv.begin("admin", "1234");
                Serial.println("LittleFS opened!");
            }
            
            #ifdef LED_HANDLER_H 
            // 5초에 100ms씩 2번 점멸
            led.set(5000, 100, 2);
            #endif
            
            isConnected = true;
            isConnecting = false;
        }
    }
    
    // 연결 중인데 타임아웃 발생 시 -> 비번 틀린 거로 간주
    if (isConnecting && connect_timeout_Timer.isReady()) {
        Serial.printf("\n%s - 연결 타임아웃.\n", current_info.ssid.c_str());

        // 완전한 중단
        WiFi.disconnect(true, true);
        WiFi.mode(WIFI_OFF);

        // 가장 중요한 과정으로, 현재 비밀번호가 틀린 WiFi정보를 비활성화
        Serial.printf("[AUTH-FAIL] %s → 영구 차단\n", current_info.ssid);
        env.wifi_list[current_info.ssid][EnvData::STATUS].set(AUTH_WRONG);
        
        current_info.ssid = "";
        current_info.password= "";
        
        isConnecting = false;  // 타임아웃이므로 다시 false
        
        #ifdef LED_HANDLER_H 
        led.set(2000, 50, 5);  // 점멸
        #endif
        
        connect_timeout_Timer.reset();
    } 
    
    // MQTT접속에 실패 시
    if (!mqtt_client.connected() && reconnectMQTT_Timer.isReady()) {
        if (WiFi.status() != WL_IDLE_STATUS && WiFi.isConnected()) reconnect(); // WiFi연결 상태에서만 재연결
        
        reconnectMQTT_Timer.reset();
    }
    
    if (isConnected) {
        mqtt_client.loop();
        ftpSrv.handleFTP();
    }
}

/////////////////////////////////// 일반 함수 들

void _callback(FtpOperation ftpOperation, unsigned int freeSpace, unsigned int totalSpace) {
    switch (ftpOperation) {
        case FTP_CONNECT:
            Serial.println(F("FTP: Connected!"));
            break;
        case FTP_DISCONNECT:
            Serial.println(F("FTP: Disconnected!"));
            break;
        case FTP_FREE_SPACE_CHANGE:
            Serial.printf("FTP: Free space change, free %u of %u!\n", freeSpace, totalSpace);
            break;
        default:
            break;
  }
}

void _transferCallback(FtpTransferOperation ftpOperation, const char* name, unsigned int transferredSize) {
  switch (ftpOperation) {
    case FTP_UPLOAD_START:
      Serial.println(F("FTP: Upload start!"));
      break;
    case FTP_UPLOAD:
      Serial.printf("FTP: Upload of file %s byte %u\n", name, transferredSize);
      break;
    case FTP_TRANSFER_STOP:
      Serial.println(F("FTP: Finish transfer!"));
      break;
    case FTP_TRANSFER_ERROR:
      Serial.println(F("FTP: Transfer error!"));
      break;
    default:
      break;
  }
}

void mqtt_callback(char* topic, uint8_t* payload, unsigned int length) {
    String recv;

    for (int i = 0; i < length; i++) recv += (char)payload[i];
    
    Serial.printf("Message arrived [%s] > %s\n", topic, recv.c_str());
    
    net.set_mqtt_recv(recv);
}

#endif
