#include "door.h"

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

void Security::unlock() {
    this->setLock(false);
}

void Security::setLock(bool locked) {
    this->_locked = locked;
    EEPROM.write(EEPROM_ADDR_LOCKED, locked ? LOCKED : UNLOCKED);
    EEPROM.commit();
}