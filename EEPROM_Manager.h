// eeprom_manager.h
#ifndef EEPROM_MANAGER_H
#define EEPROM_MANAGER_H

#include "Param_types.h"

class EEPROMManager {
public:
    void begin();
    bool saveConfig();
    bool loadConfig();
    void resetToDefaults();
    
private:
    bool shouldSaveParam(const ConfigParam& param);
};

#endif