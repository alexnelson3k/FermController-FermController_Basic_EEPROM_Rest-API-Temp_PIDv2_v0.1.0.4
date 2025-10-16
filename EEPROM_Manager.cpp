#include "EEPROM_Manager.h"
#include "Param_helpers.h"
#include <EEPROM.h>

#define EEPROM_SIZE 4096
#define CONFIG_MAGIC 0xFEED1234

extern EEPROMManager eepromManager;

struct EEPROMHeader {
    uint32_t magic;
    uint32_t version;
    uint32_t checksum;
};

void EEPROMManager::begin() {
    EEPROM.begin(EEPROM_SIZE);
}

bool EEPROMManager::saveConfig() {
    EEPROMHeader header = {
        .magic = CONFIG_MAGIC,
        .version = getParamUint8(PARAM_VERSION),
        .checksum = 0
    };
    
    // Calculate checksum (упрощенный но корректный)
    uint32_t checksum = 0;
    for (int i = 0; i < PARAM_COUNT; i++) {
        if (shouldSaveParam(system_params[i])) {
            // Checksum для разных типов данных
            switch(system_params[i].type) {
                case TYPE_FLOAT:
                    checksum += (uint32_t)(system_params[i].number.value * 1000); // float to int
                    break;
                case TYPE_UINT8:
                    checksum += system_params[i].uint8.value;
                    break;
                case TYPE_UINT16:
                    checksum += system_params[i].uint16.value;
                    break;
                case TYPE_INT16:
                    checksum += system_params[i].int16.value;
                    break;
                case TYPE_BOOL:
                    checksum += system_params[i].boolean.value ? 1 : 0;
                    break;
                case TYPE_STRING:
                    // Для строк считаем хеш первых нескольких символов
                    for (int j = 0; j < 8 && system_params[i].string.value[j] != '\0'; j++) {
                        checksum += system_params[i].string.value[j];
                    }
                    break;
            }
            checksum += i; // Добавляем индекс для уникальности
        }
    }
    header.checksum = checksum;
    
    // Save header
    EEPROM.put(0, header);
    
    // Save parameters - ВСЕ которые должны сохраняться
    int addr = sizeof(EEPROMHeader);
    for (int i = 0; i < PARAM_COUNT; i++) {
        if (shouldSaveParam(system_params[i])) {
            EEPROM.put(addr, system_params[i]);
            addr += sizeof(ConfigParam);
            
            Serial.printf("Saved: %s to addr %d\n", system_params[i].name, addr - sizeof(ConfigParam));
        }
    }
    
    bool result = EEPROM.commit();
    Serial.printf("EEPROM save %s, checksum: %lu\n", result ? "OK" : "FAILED", checksum);
    return result;
}

bool EEPROMManager::loadConfig() {
    EEPROMHeader header;
    EEPROM.get(0, header);
    
    Serial.printf("EEPROM header: magic=0x%08lX, version=%lu, checksum=%lu\n", 
                  header.magic, header.version, header.checksum);
    
    if (header.magic != CONFIG_MAGIC) {
        Serial.println("No valid config found, using defaults");
        return false;
    }
    
    // Verify checksum
    uint32_t currentChecksum = 0;
    int addr = sizeof(EEPROMHeader);
    
    // First pass: calculate checksum и найти адреса
    for (int i = 0; i < PARAM_COUNT; i++) {
        if (shouldSaveParam(system_params[i])) {
            ConfigParam storedParam;
            EEPROM.get(addr, storedParam);
            
            // Checksum как в saveConfig()
            switch(storedParam.type) {
                case TYPE_FLOAT:
                    currentChecksum += (uint32_t)(storedParam.number.value * 1000);
                    break;
                case TYPE_UINT8:
                    currentChecksum += storedParam.uint8.value;
                    break;
                case TYPE_UINT16:
                    currentChecksum += storedParam.uint16.value;
                    break;
                case TYPE_INT16:
                    currentChecksum += storedParam.int16.value;
                    break;
                
                case TYPE_BOOL:
                    currentChecksum += storedParam.boolean.value ? 1 : 0;
                    break;
                case TYPE_STRING:
                    for (int j = 0; j < 8 && storedParam.string.value[j] != '\0'; j++) {
                        currentChecksum += storedParam.string.value[j];
                    }
                    break;
            }
            currentChecksum += i;
            
            addr += sizeof(ConfigParam);
        }
    }
    
    if (currentChecksum != header.checksum) {
        Serial.printf("Checksum mismatch: stored=%lu, calculated=%lu\n", 
                      header.checksum, currentChecksum);
        return false;
    }
    
    // Second pass: load parameters
    addr = sizeof(EEPROMHeader);
    for (int i = 0; i < PARAM_COUNT; i++) {
        if (shouldSaveParam(system_params[i])) {
            ConfigParam storedParam;
            EEPROM.get(addr, storedParam);
            
            // Copy value to current parameter
            switch (system_params[i].type) {
                case TYPE_FLOAT:
                    system_params[i].number.value = storedParam.number.value;
                    break;
                case TYPE_UINT8:
                    system_params[i].uint8.value = storedParam.uint8.value;
                    break;
                case TYPE_UINT16:
                    system_params[i].uint16.value = storedParam.uint16.value;
                    break;
                case TYPE_INT16:
                    system_params[i].int16.value = storedParam.int16.value;
                    break;
                case TYPE_BOOL:
                    system_params[i].boolean.value = storedParam.boolean.value;
                    break;
                case TYPE_STRING:
                    strncpy(system_params[i].string.value, storedParam.string.value, 
                            system_params[i].string.max_size);
                    system_params[i].string.value[system_params[i].string.max_size - 1] = '\0';
                    break;
            }
            
            Serial.printf("Loaded: %s = ", system_params[i].name);
            switch (system_params[i].type) {
                case TYPE_FLOAT: Serial.println(system_params[i].number.value); break;
                case TYPE_UINT8: Serial.println(system_params[i].uint8.value); break;
                case TYPE_BOOL: Serial.println(system_params[i].boolean.value ? "true" : "false"); break;
                default: Serial.println("..."); break;
            }
            
            addr += sizeof(ConfigParam);
        }
    }
    
    Serial.println("Config loaded successfully from EEPROM");
    return true;
}

bool EEPROMManager::shouldSaveParam(const ConfigParam& param) {
    return !(param.flags & NO_FLASH_SAVE);
}

void EEPROMManager::resetToDefaults() {
    for (int i = 0; i < PARAM_COUNT; i++) {
        switch (system_params[i].type) {
            case TYPE_FLOAT:
                system_params[i].number.value = system_params[i].number.default_value;
                break;
            case TYPE_UINT8:
                system_params[i].uint8.value = system_params[i].uint8.default_value;
                break;
            case TYPE_UINT16:
                system_params[i].uint16.value = system_params[i].uint16.default_value;
                break;
            case TYPE_INT16:
                system_params[i].int16.value = system_params[i].int16.default_value;
                break;
            case TYPE_BOOL:
                system_params[i].boolean.value = system_params[i].boolean.default_value;
                break;
            case TYPE_STRING:
                strncpy(system_params[i].string.value, system_params[i].string.default_value, 
                        system_params[i].string.max_size);
                system_params[i].string.value[system_params[i].string.max_size - 1] = '\0';
                break;
        }
    }
}