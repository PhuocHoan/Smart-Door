#include "door.h"
#include "light.h"

// keypad
char keys[ROWS][COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};
byte colPins[COLS] = {16, 17, 5, 18}; // Pins connected to C1, C2, C3, C4
byte rowPins[ROWS] = {15, 2, 0, 4}; // Pins connected to R1, R2, R3, R4

// icon (bitmap icon)
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

// door
int last_update = 0; // last time update
int last_type = 0; // last time type key
byte fail_times = 0; // number of times fail to open door

// upperbound and lowerbound of temperature
byte max_temp = 40; // default is 40
byte min_temp = 15; // default is 15
bool flag = false; // use once in program to publish starter info to MQTT server in the beginning
bool pass = true;

Security security;
Servo servo;
U8X8_SSD1306_128X64_NONAME_HW_I2C display(U8X8_PIN_NONE);
DHT dht(DHT_PIN, DHT22);
Keypad keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
TM1637 tm(CLK_PIN, DIO_PIN); // tm
Light l;
RTC_DS1307 rtc;

void wifiConnect() {
    WiFi.begin(SSID, PASSWORD, WIFI_CHANNEL);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print("."); 
    }
    Serial.println(" Connected!");
}

void mqttConnect() {
    while(!mqttClient.connected()) {
        Serial.println("Attemping MQTT connection...");
        String clientId = "ESP32Client-";
        clientId += String(random(0xffff), HEX);
        if(mqttClient.connect(clientId.c_str())) {
            Serial.println("connected");

            //***Subscribe all topic you need***
            mqttClient.subscribe("/group_21/web/temperature/upper_bound");
            mqttClient.subscribe("/group_21/web/temperature/lower_bound");
            mqttClient.subscribe("/group_21/web/door");
        }
        else {
            Serial.println("try again in 5 seconds");
            delay(5000);
        }
    }
}

//MQTT Receiver
void callback(char* topic, byte* message, unsigned int length) {
    Serial.println(topic);
    String strMsg;
    for(int i=0; i<length; ++i) {
        strMsg += (char)message[i];
    }
    Serial.println(strMsg);

    // ***Code here to process the received package***
    if (!strcmp(topic, "/group_21/web/temperature/upper_bound")) {
        max_temp = (byte)strMsg.toInt();
    } else if (!strcmp(topic, "/group_21/web/temperature/lower_bound")) {
        min_temp = (byte)strMsg.toInt();
    } else if (!strcmp(topic, "/group_21/web/door")) {
        bool tmp = strMsg.toInt();
        if (tmp) {
            security.unlock();
            unlock();
            pass = true;
            fail_times = 0;
        } else {
            lock();
        }
    }
}

void displayTime(RTC_DS1307 rtc) {
    DateTime now = rtc.now();
    tm.display(0, int(now.hour()/10));
    tm.display(1, now.hour()%10);
    tm.display(2, int(now.minute()/10));
    tm.display(3, now.minute()%10);
    tm.point(true);
}

void starterPublish() {
    mqttClient.publish("/group_21/local/temperature/upper_bound", String(max_temp).c_str());
    mqttClient.publish("/group_21/local/temperature/lower_bound", String(min_temp).c_str());
    if (security.locked()) {
        lock();
        mqttClient.publish("/group_21/local/door_lock", String(1).c_str());
    } else {
        unlock();
        mqttClient.publish("/group_21/local/door_unlock", String(1).c_str());
    }
}

// tone function manually to prevent conflict in pwm signal between (servo and buzzer)
/*
    Cách hoạt động của hàm này:
    - piezo phát ra âm thanh là do sự va chạm của dòng điện vào lớp màng trên piezo.
      Tốc độ dòng điện đập vào màng càng nhanh thì âm thanh có tần số càng lớn (nghe càng chói tai).
    - Trong hàm này, period (chu kỳ) = 1 / f (tần số), ta quy đổi 1s sang 1000000 microsecond để tạo ra tần số cao
      chính xác hơn. delayMicroseconds(500) thay vì delay(0.5)
    - ta cần halfPeriod để delay 2 lần trong mỗi lần digitalWrite(HIGH/LOW) tượng trưng khi dòng điện đi qua màng 
      tắt mở với khoảng delay microsecond thì màng bung ra và thu lại tốc độ cao mới tạo ra tần số được.
    - halfPeriod * 2 * (duration * 1000) / period = duration thực tế chạy. Do đó (duration * 1000) / period là số vòng lặp cần thực hiện
*/
void playTone(int frequency, int duration) {
    int period = 1000000 / frequency;  // Period in microseconds
    int halfPeriod = period / 2;       // Half of the period

    for (int i = 0; i < (duration * 1000) / period; ++i) {
        digitalWrite(BUZZER_PIN, HIGH);
        delayMicroseconds(halfPeriod);
        digitalWrite(BUZZER_PIN, LOW);
        delayMicroseconds(halfPeriod);
    }
}

void lock() {
    servo.write(SERVO_LOCK_POS);
    security.lock();
}

void unlock() {
    servo.write(SERVO_UNLOCK_POS);
}

void showStartupMessage() {
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

void showHomeScreen() {
    display.clearDisplay();
    int temp = dht.readTemperature(),
        humid = dht.readHumidity();
    String temperature = "Temp: " + String(temp) + "°C",
           humidity = "Humid: " + String(humid) + "%";
    display.drawUTF8(1, 0, temperature.c_str());
    display.drawString(1, 2, humidity.c_str());

    // upload data to mqtt server
    mqttClient.publish("/group_21/local/temperature/temperature", String(temp).c_str());
    mqttClient.publish("/group_21/local/humidity", String(humid).c_str());

    // // send warning to mqtt server
    if (temp > max_temp) {
        mqttClient.publish("/group_21/local/temperature/warning", String("high").c_str()); // temperature exceeds threshold
    } else if (temp < min_temp) {
        mqttClient.publish("/group_21/local/temperature/warning", String("low").c_str()); // temperature fall behind threshold
    }
}

void showUnlockMessage() {
    display.clearDisplay();
    display.drawTile(0, 0, 1, iconUnlocked);
    display.drawTile(15, 0, 1, iconUnlocked);
    display.drawString(4, 0, "Unlocked!");
    playTone(1000, 1000);
}

String inputSecretCode() {
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

bool setNewCode() {
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

void unlockedLogic() {
    display.clearDisplay();

    display.drawTile(0, 0, 1, iconUnlocked);
    display.drawString(3, 0, "# To lock");
    display.drawTile(15, 0, 1, iconUnlocked);

    bool newCodeNeeded = true; // in case user has no passcode yet

    if (security.hasCode()) {
        display.drawString(2, 2, "A = new code");
        newCodeNeeded = false;
    }

    char key;
    while (key != 'A' && key != '#') {
        key = keypad.getKey();
         if (millis() - last_type > LIMIT_TIME) {
            lock();
            mqttClient.publish("/group_21/local/door_lock", String(1).c_str());
            return;
        }
    }
    last_type = millis();
    bool readyToLock = true;
    if (key == 'A' || newCodeNeeded) {
        readyToLock = setNewCode();
    }

    if (readyToLock) {
        display.clearDisplay();
        display.drawTile(5, 0, 1, iconUnlocked);
        display.drawString(6, 0, " -> ");
        display.drawTile(10, 0, 1, iconLocked);
        lock();
        mqttClient.publish("/group_21/local/door_lock", String(1).c_str());
        delay(1000);
    }
}

void lockedLogic() {
    display.clearDisplay();
    display.drawTile(0, 0, 1, iconLocked);
    display.drawString(2, 0, "Door Locked!");
    display.drawTile(15, 0, 1, iconLocked);

    String code = inputSecretCode();
    if (code.length() != CODE_LEN) return;
    bool unlockSuccessfully = security.unlock(code);

    if (unlockSuccessfully) {
        showUnlockMessage();
        unlock();
        mqttClient.publish("/group_21/local/door_unlock", String(1).c_str()); // 1: close door successfully
        fail_times = 0;
    } else {
        display.clearDisplay();
        display.setCursor(0, 0);

        // wrong passcode for 3 times
        if (++fail_times == LIMIT_FAIL_TIMES) {
            display.drawString(0, 0, "YOU ARE STRANGER");
            display.drawString(0, 3, "WHO ARE YOU!!!");
            playTone(2000, 3000);
            
            // send warning to mqtt server
            mqttClient.publish("/group_21/local/door_unlock", String(0).c_str()); // 0: open door fail (3 times), and warn to user
            
            // have to open door from web
            pass = false; // to prevent opening door if not open by web
        } else {
            display.drawString(2, 0, "Access Denied!");
            delay(1000);
        }
    }
}

void doorLogic() {
    if (!flag) {
        starterPublish();
        flag = true;
    }

    char key = keypad.getKey();
    // type any key to see mode 'open door'.
    if (((key >= '0' && key <= '9') || (key >= 'A' && key <= 'D') || key == '*' || key == '#') && pass == true) {
        last_type = millis();
        showStartupMessage();
        while (millis() - last_type < LIMIT_TIME && pass == true) {
            if (security.locked()) {
                lockedLogic();
            } else {
                unlockedLogic();
            }
        }
    } else {
        if (millis() - last_update > 5000) {
            showHomeScreen();
            // update every 5s
            last_update = millis();
        }
    }
}

void setup() {
    Serial.begin(115200);
    Serial.print("Connecting to WiFi");
    wifiConnect();
    if (!rtc.begin()) {
        Serial.println("Couldn't find RTC");
        Serial.flush();
        abort();
    }
    l.init();
    tm.init();
    tm.set(BRIGHT_TYPICAL);
    pinMode(BUZZER_PIN, OUTPUT);
    servo.attach(SERVO_PIN);
    dht.begin();
    display.begin();
    display.setFont(u8x8_font_8x13_1x2_f);

    mqttClient.setServer(MQTT_SERVER, PORT);
    mqttClient.setCallback(callback);
    mqttClient.setKeepAlive(90);

    showHomeScreen();
}

void loop() {
    // reconnect mqtt if interrupted
    if (!mqttClient.connected()) {
        mqttConnect();
    }
    mqttClient.loop();

    doorLogic();
    displayTime(rtc);
    l.run();
}