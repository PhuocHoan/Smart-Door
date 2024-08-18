#include "door.h"
#include "light.h"

Door door;
Light l;
RTC_DS1307 rtc;

void setup() {
    Serial.begin(115200);
    if (!rtc.begin()) {
        Serial.println("Couldn't find RTC");
        Serial.flush();
        abort();
    }
    door.init();
    l.init();
}

void loop() {
    door.doorLogic();
    door.displayTime(rtc);
    l.run();
}