#include "door.h"

Door door;

void setup() {
    Serial.begin(115200);
    door.init();
}

void loop() {
    door.doorLogic();
}