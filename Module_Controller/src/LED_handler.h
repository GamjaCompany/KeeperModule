#ifndef LED_HANDLER_H
#define LED_HANDLER_H

/* 개요: LED 동작을 관리하는 헤더 입니다.
 * --------------------------------------------
 * 1. 메인, 점멸 타이머로 구성되있습니다
 * 2. 일정하게 점멸하는 것과 중간에 빠르게 여러번 점멸하는 것을 지원합니다
*/

#include <Arduino.h>
#include <SimpleTimer.h>
#include <HW_config.h>
#define dW digitalWrite
#define NOT_USE_BLINK -1

class LED_handler {
    private:
        SimpleTimer mainTimer;
        SimpleTimer blinkTimer;
        int blink_cnt;
        int cur_blink_cnt;
        bool stat;
        bool prev_stat;
    public:
        LED_handler() = default;
        LED_handler& operator=(const LED_handler& ref) = delete;
        static LED_handler& GetInstance();
        void init();
        
        void set(int main_interval, int blink_cnt);
        
        void set(int main_interval, int blink_interval, int blink_cnt);
        
        void run();
        
};

LED_handler& LED_handler::GetInstance() {
    static LED_handler instance;
    
    return instance;
}

void LED_handler::init() {
    mainTimer.setInterval(1000);
    blinkTimer.setInterval(50);
    blink_cnt = NOT_USE_BLINK;
    stat = false;
    prev_stat = false;
}


void LED_handler::set(int main_interval, int blink_cnt) {
    mainTimer.setInterval(main_interval);
    this->cur_blink_cnt = this->blink_cnt = NOT_USE_BLINK;
    
    stat = false;
    dW(BUILTIN_LED, LOW);
}

void LED_handler::set(int main_interval, int blink_interval, int blink_cnt) {
    mainTimer.setInterval(main_interval);
    blinkTimer.setInterval(blink_interval);
    this->cur_blink_cnt = this->blink_cnt = blink_cnt*2;
    
    stat = false;
    dW(BUILTIN_LED, LOW);
}

void LED_handler::run() {           
    if (mainTimer.isReady()) {
        if (cur_blink_cnt <= 0) cur_blink_cnt = blink_cnt;
        
        // 점멸 모드를 안 쓸 경우
        if (blink_cnt == NOT_USE_BLINK)
            stat = !stat;

        mainTimer.reset();
    }
    
    if (0 < cur_blink_cnt && blinkTimer.isReady()) {
        stat = !stat;
        
        cur_blink_cnt--;
        
        blinkTimer.reset();
    }
    
    if(prev_stat != stat) dW(BUILTIN_LED, stat);
    
    prev_stat = stat;
}

LED_handler& led = LED_handler::GetInstance();

#endif