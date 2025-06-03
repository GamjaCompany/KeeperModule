#ifndef HW_CONFIG_H
#define HW_CONFIG_H

#include <Arduino.h>
// #include <LED_handler.h>
#include <AccelStepper.h>
#include <SimpleTimer.h>

#define LED_BUILTIN 2
#define dW digitalWrite

#define IN1 12
#define IN2 13
#define IN3 15
#define IN4 14

#define CW true
#define CCW false

const int stepsPerRevolution = 2048;

// Stepper mStepper(stepsPerRevolution, IN1, IN3, IN2, IN4);

class Step_motor_handler {
    private:
        AccelStepper myStepper;

        SimpleTimer t1;

        bool isActive;
        bool direction;
    public:
        Step_motor_handler(): myStepper(AccelStepper::FULL4WIRE, IN1, IN3, IN2, IN4) {}
        Step_motor_handler& operator=(const Step_motor_handler& ref) = delete;  
        static Step_motor_handler& GetInstance();

        void init();
        void activate();
        void deactivate();
        void run();
};

Step_motor_handler& Step_motor_handler::GetInstance() {
    static Step_motor_handler instance;
    
    return instance;
}

void Step_motor_handler::init() {
    myStepper.setMaxSpeed(500);     // 최대 속도 (steps/sec)
    myStepper.setAcceleration(100); // 가속도 (steps/sec^2)
    // myStepper.moveTo(2048);         // 한 바퀴 (예: 2048 steps)

    t1.setInterval(10);

    isActive = false;
    direction = CW;
}

void Step_motor_handler::activate() {
    // myStepper.moveTo(-myStepper.currentPosition());
    direction = !direction;

    // 바뀐 방향 Serial로 출력
    Serial.printf("Direction: %s\n", direction ? "CCW" : "CW");

    myStepper.setSpeed(direction ? 500 : -500);
    // isActive = stat;
}

void Step_motor_handler::deactivate() {
    // myStepper.moveTo(-myStepper.currentPosition());
    myStepper.setSpeed(0);
    // isActive = stat;
}

void Step_motor_handler::run() {
    // if (t1.isReady()) {
    //     myStepper.step(512);
        
    //     t1.reset();
    // }
    myStepper.runSpeed();
} Step_motor_handler& step_motor = Step_motor_handler::GetInstance();

void hw_init() {
    pinMode(LED_BUILTIN, OUTPUT); dW(LED_BUILTIN, LOW);

    step_motor.init();

    // mStepper.setSpeed(5);
    
    #ifdef LED_HANDLER_H
    led.init();
    #endif
}

#endif