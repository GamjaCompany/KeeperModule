#ifndef MQTTHANDLER_H
#define MQTTHANDLER_H

#include "env.h"
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

extern void handleRecv(String topic, String payload);

/**MQTT메시지 수신 시 콜백처리 함수 */
void callback(char* topic, byte* payload, unsigned int length);
/**수신 MQTT메시지 중 토픽 해석 함수*/
Topic_CMD getCMD_idx(String topic);
/**연결 실패 시 재접속 기능*/
void reconnect();

WiFiClientSecure espClient;
PubSubClient client(espClient);

/**최초 네트워크 접속 및 MQTT브로커 서버 접속 기능*/
void setup_wifi() {
    unsigned long trig = millis();

    String ssid="", password="";

    // 10초동안 신규 와이파이정보를 입력할 수 있음
    // 다른 네트워크 환경이라면 이때 SSID/PW 입력가능
    Serial.print("Input the SSID/PW : ");
    while(millis() - trig <= 10000) {
        if(Serial.available()) {
            String recv = Serial.readStringUntil('\n');

            if(recv.indexOf("/") == -1) break;

            String ssid = recv.substring(0, recv.indexOf('/'));
            String pw = recv.substring(recv.indexOf('/')+1, recv.length());
        }
    }

    Serial.print("Connecting to ");

    // 파싱한 결과가 1글자보다 많으면 신규입력 정보로 연결시도
    if(1 < ssid.length()) {
        Serial.println(ssid);

        WiFi.persistent(false);
        WiFi.begin(ssid, password);
    } else {  // 아니면 env.h에 설정해둔 정보로 연결시도
        Serial.println(project_env::ssid);

        WiFi.persistent(false);
        WiFi.begin(project_env::ssid, project_env::password);
    }

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());


    espClient.setInsecure();  // SSL보안을 실제로 쓰지는 않음
    client.setServer(project_env::broker_address, project_env::broker_port);
    client.setCallback(callback);
}

void callback(char* topic, byte* payload, unsigned int length) {
    static String msg;

    // 메시지를 char* 에서 String으로 변환
    msg = "";
    for (int i=0; i < length; i++) msg += (char)payload[i];

    // 수신받은 메시지 해석시작
    handleRecv( String(topic), msg ); 
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect(project_env::broker_address, project_env::broker_id, project_env::broker_pw)) {
      Serial.println("MQTT서버 연결됨!");

      client.subscribe("GET");      // 앱/웹 에서 말뚝 정보 요청할 때 사용
      client.subscribe("Control");  // 앱/웹 에서 말뚝 수동조종 시 사용
      client.subscribe("DEBUG");  // 개발 목적용도
      client.subscribe("AI");     // AI중앙 컴퓨터에서 제어목적

    } else {
      Serial.printf("MQTT서버 실패, rc= %d || 3초 후에 다시 연결 시도함\n", client.state());
      delay(3000);
    }
  }
}

Topic_CMD getCMD_idx(String topic) {
    if(topic == "Control") return CONTROL;
    if(topic == "GET")     return GET;
    if(topic == "DEBUG")   return DEBUG;
    if(topic == "AI")      return AI;

    return NONE;
}

#endif