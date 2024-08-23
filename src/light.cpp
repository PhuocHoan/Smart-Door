#include "light.h"

void Light::on() {
    digitalWrite(RELAY_PIN,HIGH);
}

void Light::off() {
    digitalWrite(RELAY_PIN,LOW);
}

void Light::run() {
    int light_sensor_value = analogRead(LDR_PIN);
    int motion = digitalRead(PIR_PIN);
    unsigned long currentTime = millis();
  
    if (light_sensor_value > 2500) {
    // Daytime
        off();
        state = false;
    } else {
    // Nighttime
    if (motion == HIGH) {
        on(); // Light on if motion detected
        lastMotionTime = currentTime;
        state = true;
        Serial.println("Motion detected");
    } else {
      // No motion detected
        if (state && (currentTime - lastMotionTime) > motionDelay) {
            off(); // Turn off light after delay
            state = false;
        } 
    }
  }
  delay(100);
}