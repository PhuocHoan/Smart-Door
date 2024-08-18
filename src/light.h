#include <Arduino.h>

#define LDR_PIN 14
#define PIR_PIN 26
#define RELAY_PIN 27

class Light {
    private:  
        bool state;
        unsigned long lastMotionTime;
        int motionDelay;
    public:
        Light();
        void init() {
            state = false;        
            lastMotionTime = 0;    
            motionDelay = 5000;    
            pinMode(RELAY_PIN,OUTPUT);
            pinMode(PIR_PIN,INPUT);
            pinMode(LDR_PIN, INPUT);
        }
        void setState(bool b) {
            state = b;
        }
        bool getState() const {
            return state;
        }
        void setLastMotionTime(const unsigned long n) {
            lastMotionTime = n;
        }
        unsigned long getLastMotionTime() const {
            return lastMotionTime;
        }
        // Getter and Setter for motionDelay
        void setMotionDelay(const int n) {
            motionDelay = n;
        }
        int getMotionDelay() const {
            return motionDelay;
        }
        void on();
        void off();
        void run();
};