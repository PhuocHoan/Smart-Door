#ifndef DOOR_H
#define DOOR_H

#include <Arduino.h>
#include <DHT.h>
#include <Keypad.h>
#include <U8g2lib.h>
#include <ESP32Servo.h>
#include <EEPROM.h> // the library for saving last state of components, store value in nvs flash
#include <nvs_flash.h>
#include "RTClib.h"
#include "TM1637.h"

// button
#define BUTTON 23

// dht
#define DHT_PIN 19

// keypad
#define ROWS 4
#define COLS 4

extern char keys[ROWS][COLS];
extern byte rowPins[ROWS];
extern byte colPins[COLS];
extern uint8_t iconLocked[8] PROGMEM;
extern uint8_t iconUnlocked[8] PROGMEM;

// Servo
#define SERVO_PIN 13
#define SERVO_LOCK_POS 0
#define SERVO_UNLOCK_POS 90

// buzzer
#define BUZZER 12

// RTC


// TM1637
#define DIO_PIN 32
#define CLK_PIN 33

/*
    Security handling
    password has code's size: 4 numbers
*/
#define CODE_LEN 4
#define EEPROM_SIZE 512
#define EEPROM_ADDR_LOCKED 0 // store state of door (locked / unlocked)
#define EEPROM_ADDR_CODE_LEN 1 // store code length: 4 (default)
#define EEPROM_ADDR_CODE 2 // store 4 numbers in password code
#define EEPROM_EMPTY 0xff // empty value

#define UNLOCKED '0'
#define LOCKED '1'
#define LIMIT_FAIL_TIMES 3

// time
#define LIMIT_TIME 10000 // 10s

class Security {
    private:
        bool _locked;
        void setLock(bool locked);
    public:
        Security(); // get the last state before power off
        void lock(); // set to locked state
        bool unlock(String code); // to check some condition before setting unlocked state
        bool locked(); // check if locked or unlocked?
        bool hasCode(); // check if code already exists? 
        void setCode(String newCode); // set and store new code
};

class Door {
    private:
        // Servo servo; // servo
        // U8X8_SSD1306_128X64_NONAME_HW_I2C display; // Screen
        // DHT dht; // dht
        // Keypad keypad; // keypad
    public:
        Security security;
        Servo servo; // servo
        U8X8_SSD1306_128X64_NONAME_HW_I2C display; // Screen
        DHT dht; // dht
        Keypad keypad; // keypad
        TM1637 tm; // tm
        Door(); // initialize constructor for variables
        void init(); // initialize variables
        void lock(); // lock the door
        void unlock(); // unlock the door
        void showStartupMessage();
        String inputSecretCode(); // get user input-code
        bool setNewCode();
        void showUnlockMessage();
        void safeUnlockedLogic(); // logic when door is unlocked
        void safeLockedLogic(); // logic when door is locked
        void showStaticScreen(); // screen show when someone does not type key to open door.
        void doorLogic(); // handle all logic in door.
        void displayTime(RTC_DS1307 rtc);
};

/* Some function for door */

#endif