#ifndef ENV_H
#define ENV_H

/* 개요: env.txt로부터 개인정보를 추출, 전처리하는 헤더 입니다.
 * --------------------------------------------
 * 1. LittleFS를 사용합니다.
 * 2. ArduJson을 사용하여 파싱 후 관리합니다.
*/

#include <ArduinoJson.h>
#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>
#define FOR(i, b, e) for(int i = b; i < e; i++)

// 브로커 서버관련 정보
typedef struct MQTT_info {
    const char* broker_address;
    int broker_port;
    const char* user_id;
    const char* user_password;  // 암호화 여부
} MQTT_info;

class EnvData {
    private:
        JsonDocument raw;
        
    public:
        enum {
            PASSWORD=0, STATUS=1  
        };
        JsonObject wifi_list;
        String name;
        MQTT_info mqtt;
        
        EnvData& operator=(const EnvData& ref) = delete;  
        static EnvData& GetInstance();
        void init();
        void print_wifi_list();
        void print_mqtt();
        String fileLoad();
        String getName();
        
};

EnvData& EnvData::GetInstance() {
    static EnvData instance;
    
    return instance;
}

String EnvData::fileLoad() {           
    while (!LittleFS.begin(true)) {
        Serial.println("An Error has occurred while mounting LittleFS");
        delay(500);
    }
    
    File file = LittleFS.open("/env.txt");
    String content = "";
    
    if(!file) {
        Serial.println("Failed to open file for reading");
        return String();
    }

    if(file.available()) content = file.readString();
    
    Serial.printf("content: %s\n", content.c_str());
    
    file.close();
    
    // 파일에서 읽은 데이터를 가지고 env객체 초기화
    return content;
}

void EnvData::init() {
    name = "NULL";
    
    String load_data = fileLoad();
    
    DeserializationError err = deserializeJson(raw, load_data);
    
    if (err) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(err.f_str());
        return;
    }
    
    wifi_list = raw["wifi"].as<JsonObject>();

    name = raw["name"].as<String>();
    
    mqtt.broker_address = raw["mqtt"]["broker_address"];
    mqtt.broker_port    = raw["mqtt"]["port"];
    mqtt.user_id        = raw["mqtt"]["user_id"];
    mqtt.user_password  = raw["mqtt"]["user_password"];
    
    // 정리한 내용 출력
    print_wifi_list();
    print_mqtt();
}

void EnvData::print_wifi_list() {
    for(JsonPair pair : wifi_list) {
        const char* ssid     = pair.key().c_str();
        const char* password = pair.value().as<JsonArray>()[0];
        
        Serial.print("SSID: ");
        Serial.print(ssid);
        Serial.print(", Password: ");
        Serial.println(password);
    }

}

void EnvData::print_mqtt() {
    Serial.print("Broker Address: ");
    Serial.println(mqtt.broker_address);
    Serial.print("Port: ");
    Serial.println(mqtt.broker_port);
    Serial.print("User ID: ");
    Serial.println(mqtt.user_id);
    Serial.print("User Password: ");
    Serial.println(mqtt.user_password);
}

String EnvData::getName() {
    return name;
}

EnvData& env = EnvData::GetInstance();

#endif
