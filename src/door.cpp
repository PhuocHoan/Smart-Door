#include "door.h"

// keypad
char keys[ROWS][COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};
byte colPins[COLS] = {16, 17, 5, 18}; // Pins connected to C1, C2, C3, C4
byte rowPins[ROWS] = {15, 2, 0, 4}; // Pins connected to R1, R2, R3, R4

// icon
uint8_t iconLocked[8] PROGMEM = {
    0b1111110,
    0b1111001,
    0b1001001,
    0b1111001,
    0b1111110,
    0b0,
    0b0,
    0b0
};

uint8_t iconUnlocked[8] PROGMEM = {
    0b1111110,
    0b1111001,
    0b1001001,
    0b1111001,
    0b1111000,
    0b0,
    0b0,
    0b0
};

int last_update = 0;
int last_type = 0;
byte fail_times = 0;

Security::Security() {
    esp_err_t err = nvs_flash_init(); // Initialize NVS
    // handle when happen error in nvs flash memory
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        ESP_ERROR_CHECK(nvs_flash_erase());
        // Retry nvs_flash_init
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    if (!EEPROM.begin(EEPROM_SIZE)) {
        Serial.println("Failed to initialize EEPROM");
        return;
    }

    this->_locked = EEPROM.read(EEPROM_ADDR_LOCKED) == LOCKED;
}

void Security::lock() {
    this->setLock(true);
}

bool Security::locked() {
    return this->_locked;
}

bool Security::hasCode() {
    byte old_codeLength = EEPROM.read(EEPROM_ADDR_CODE_LEN);
    return old_codeLength != EEPROM_EMPTY;
}

void Security::setCode(String newCode) {
    EEPROM.write(EEPROM_ADDR_CODE_LEN, newCode.length());
    EEPROM.commit();
    for (byte i = 0; i < newCode.length(); ++i) {
        EEPROM.write(EEPROM_ADDR_CODE + i, newCode[i]);
        EEPROM.commit();
    }
}

bool Security::unlock(String code) {
    byte old_codeLength = EEPROM.read(EEPROM_ADDR_CODE_LEN);
 
    if (old_codeLength == EEPROM_EMPTY) {
        // There was no code, so always unlock
        this->setLock(false);
        return true;
    }

    if (old_codeLength != code.length()) {
        return false;
    }

    for (byte i = 0; i < old_codeLength; ++i) {
        if (code[i] != EEPROM.read(EEPROM_ADDR_CODE + i)) {
            return false;
        }
    }
    this->setLock(false);
    return true;
}

void Security::setLock(bool locked) {
    this->_locked = locked;
    EEPROM.write(EEPROM_ADDR_LOCKED, locked ? LOCKED : UNLOCKED);
    EEPROM.commit();
}

Door::Door() : display(U8X8_PIN_NONE), dht(DHT_PIN, DHT22), keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS), tm(CLK_PIN, DIO_PIN) {}


void Door::init() {
    dht.begin();
    display.begin();
    servo.attach(SERVO_PIN);
    pinMode(BUTTON, INPUT_PULLUP);
    display.setFont(u8x8_font_8x13_1x2_f);
    tm.init();
    tm.set(BRIGHT_TYPICAL);

    // Make sure the physical lock is synced with the last state
    if (security.locked()) {
        lock();
    } else {
        unlock();
    }

    showStaticScreen();
}

void Door::lock() {
    servo.write(SERVO_LOCK_POS);
    security.lock();
}

void Door::unlock() {
    servo.write(SERVO_UNLOCK_POS);
}

void Door::showStartupMessage() {
    display.clearDisplay();
    display.drawString(4, 0, "Welcome!");
    delay(1000);

    display.setCursor(0, 2);
    String message = "Group 21's house";
    for (byte i = 0; i < message.length(); ++i) {
        display.print(message[i]);
        delay(100);
    }
    delay(500);
}

String Door::inputSecretCode() {
    display.drawString(5, 2, "[____]");
    display.setCursor(6, 2);
    String code = "";
    while (code.length() != CODE_LEN) {
        char key = keypad.getKey();
        // delete all key typed
        if (key == 'C') {
            code = "";
            display.drawString(5, 2, "[____]");
            display.setCursor(6, 2);
            last_type = millis();
        } else if (key >= '0' && key <= '9') {
            display.print('*');
            code += key;
            last_type = millis();
        } else if (millis() - last_type > LIMIT_TIME) {
            break;
        }

    }
    return code;
}

bool Door::setNewCode() {
    display.clearDisplay();
    display.drawString(0, 0, "Enter new code:");
    String newCode = inputSecretCode();
    if (newCode.length() != CODE_LEN) return false;

    display.clearDisplay();
    display.drawString(0, 0, "Confirm new code");
    String confirmCode = inputSecretCode();
    if (confirmCode.length() != CODE_LEN) return false;
    if (newCode.equals(confirmCode)) {
        security.setCode(newCode);
        return true;
    } else {
        display.clearDisplay();
        display.drawString(1, 0, "Code mismatch");
        delay(1000);
        return false;
    }
}

void Door::showUnlockMessage() {
    display.clearDisplay();
    display.drawTile(0, 0, 1, iconUnlocked);
    display.drawTile(15, 0, 1, iconUnlocked);
    display.drawString(4, 0, "Unlocked!");
    delay(1000);
}

void Door::safeUnlockedLogic() {
    display.clearDisplay();

    display.drawTile(0, 0, 1, iconUnlocked);
    display.drawString(3, 0, "# To lock");
    display.drawTile(15, 0, 1, iconUnlocked);

    bool newCodeNeeded = true; // in case user has no passcode yet

    if (security.hasCode()) {
        display.drawString(2, 2, "A = new code");
        newCodeNeeded = false;
    }

    char key = keypad.getKey();
    while (key != 'A' && key != '#') {
        key = keypad.getKey();
        if (millis() - last_type > LIMIT_TIME) return;
    }

    bool readyToLock = true;
    if (key == 'A' || newCodeNeeded) {
        readyToLock = setNewCode();
    }

    if (readyToLock) {
        display.clearDisplay();
        display.drawTile(5, 0, 1, iconUnlocked);
        display.drawString(6, 0, " -> ");
        display.drawTile(10, 0, 1, iconLocked);

        security.lock();
        lock();
        delay(1000);
    }
}

void Door::safeLockedLogic() {
    display.clearDisplay();
    display.drawTile(0, 0, 1, iconLocked);
    display.drawString(2, 0, "Door Locked!");
    display.drawTile(15, 0, 1, iconLocked);

    String code = inputSecretCode();
    if (code.length() != CODE_LEN) return;
    bool unlockSuccessfully = security.unlock(code);

    if (unlockSuccessfully) {
        tone(BUZZER, 1000, 1000);
        showUnlockMessage();
        unlock();
        fail_times = 0;
    } else {
        display.clearDisplay();
        display.setCursor(0, 0);
        if (++fail_times == LIMIT_FAIL_TIMES) {
            display.drawString(0, 0, "YOU ARE STRANGER");
            display.drawString(0, 3, "WHO ARE YOU!!!");
            tone(BUZZER, 2000, 3000);
            delay(3000);
            // have to open door from web
        } else {
            display.drawString(2, 0, "Access Denied!");
            delay(1000);
        }
    }
}

void Door::showStaticScreen() {
    display.clearDisplay();
    String temperature = "Temp: " + String(int(dht.readTemperature())) + "°C",
        humidity = "Humid: " + String(int(dht.readHumidity())) + "%";
    display.drawUTF8(1, 0, temperature.c_str());
    display.drawString(1, 2, humidity.c_str());
}

void Door::doorLogic() {
    char key = keypad.getKey();
    Serial.println(key);
    // type any key to see mode 'open door'.
    if ((key >= '0' && key <= '9') || (key >= 'A' && key <= 'D') || key == '*' || key == '#') {
        showStartupMessage();
        while (millis() - last_type < LIMIT_TIME) {
            if (security.locked()) {
                safeLockedLogic();
            } else {
                safeUnlockedLogic();
            }
        }
    } else {
        if (millis() - last_update > 5000) {
            showStaticScreen();
            // update every 5s
            last_update = millis();
        }
    }
}

void Door::displayTime(RTC_DS1307 rtc) {
    DateTime now = rtc.now();
    tm.display(0, int(now.hour()/10));
    tm.display(1, now.hour()%10);
    tm.display(2, int(now.minute()/10));
    tm.display(3, now.minute()%10);
    tm.point(true);
}