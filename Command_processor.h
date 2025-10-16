#ifndef COMMAND_PROCESSOR_H
#define COMMAND_PROCESSOR_H

#include <Arduino.h>
#include "Param_types.h"

class Command_processor {
public:
    void handleSerialCommands();
    void showHelp();
    void showAllParameters();
    
private:
    bool setParameter(ConfigParam& param, const String& value);
};

#endif