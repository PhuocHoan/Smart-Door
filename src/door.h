#include <Arduino.h>
#include <DHT.h>
#include <Keypad.h>
#include <U8g2lib.h>
#include <ESP32Servo.h>
#include <EEPROM.h> // the library for saving last state of components, store value in nvs flash
#include <nvs_flash.h> // nvs flash
#include <WiFi.h>
#include <PubSubClient.h>
#include "RTClib.h"
#include "TM1637.h"

// Wifi
#define SSID "Wokwi-GUEST"
#define PASSWORD ""
// Defining the WiFi channel speeds up the connection:
#define WIFI_CHANNEL 6

// dht
#define DHT_PIN 19

// keypad
#define ROWS 4
#define COLS 4

// Set mqtt server
#define MQTT_SERVER "172.21.57.182" // my local mqtt server (you can change by your local ip that host mqtt server or any mqtt server)
// #define MQTT_SERVER "broker.hivemq.com"
#define PORT 1883

// Servo
#define SERVO_PIN 23
#define SERVO_LOCK_POS 0
#define SERVO_UNLOCK_POS 90

// TM1637
#define DIO_PIN 32
#define CLK_PIN 33

// buzzer
#define BUZZER_PIN 13

/*
    Security handling
    password has code's size: 4 numbers
*/
#define CODE_LEN 4
#define EEPROM_SIZE 512
#define EEPROM_ADDR_LOCKED 0 // store state of door (locked / unlocked)
#define EEPROM_ADDR_CODE_LEN 1 // store code length: 4 (default)
#define EEPROM_ADDR_CODE 2 // store 4 numbers in password code
#define EEPROM_EMPTY 0xff // empty value in eeprom

#define UNLOCKED '0'
#define LOCKED '1'
#define LIMIT_FAIL_TIMES 3

// time
#define LIMIT_TIME 10000 // after 10s not receive any key type, back to home screen.
#define UPDATE_TIME 5000 // update humidity, temperature every 5s

void lock();
void unlock();

class Security {
    private:
        bool _locked;
        void setLock(bool locked);
    public:
        Security(); // get the last state before power off
        void lock(); // set to locked state
        bool unlock(String code); // to check some condition before setting unlocked state
        void unlock(); // to open door from web
        bool locked(); // check if locked or unlocked?
        bool hasCode(); // check if code already exists? 
        void setCode(String newCode); // set and store new code
};