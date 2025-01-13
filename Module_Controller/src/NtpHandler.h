#ifndef NTPHANDLER_H
#define NTPHANDLER_H

#include <Arduino.h>
#include <NTPClient.h>
#include <TimeLib.h>
#include <WiFiUdp.h>

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 9 * 3600, 60000); // NTP 서버, UTC+9 시간, 60초 업데이트

char buf_time[32];  // 시간값 반환때 사용용도

void setup_ntp() {
    static bool isAlreadyBegin = false;

    // 여러 곳에서 setup하여도 한 곳에서만 setup됨.
    if(!isAlreadyBegin) timeClient.begin();

    isAlreadyBegin = true;
}

/**
 * 시스템 시간 객체를 ntp시간으로 갱신합니다.
 */
void update_ntp() {
    timeClient.update();  // ntp서버에서 현재시간 불러오기기
    
    setTime(timeClient.getEpochTime());  // time객체 현재 NTP시간으로 갱신
}

/**
 * yyyy-mm-dd 형식의 시간 값을 알려줍니다. 
 * @returns 해당 값을 가르키는 문자열 주소(char*)
 */
char* getDate() {
    memset(buf_time, 0, 32U);

    update_ntp();

    // 출력형식: 2025-01-01
    sprintf(buf_time, "%04d-%02d-%02d", year(), month(), day());

    return buf_time;
}

/**
 * hh:mm:ss 형식의 시간 값을 알려줍니다. 
 * @returns 해당 값을 가르키는 문자열 주소(char*)
 */
char* getTime() {
    memset(buf_time, 0, 32U);

    update_ntp();

    // 출력형식: 11:04:03
    sprintf(buf_time, "%02d:%02d:%02d", hour(), minute(), second());

    return buf_time;
}

/**
 * yyyy-mm-dd|hh:mm:ss 형식의 시간 값을 알려줍니다.
 * @returns 해당 값을 가르키는 문자열 주소(char*)
 */
char* getRawTime(bool choice) {
    memset(buf_time, 0, 32U);
    
    update_ntp();

    // 출력형식: 2025-01-01|11:04:03
    sprintf(buf_time, "%04d-%02d-%02d T%02d:%02d:%02d", year(), month(), day(), hour(), minute(), second());

    return buf_time;
}

#endif