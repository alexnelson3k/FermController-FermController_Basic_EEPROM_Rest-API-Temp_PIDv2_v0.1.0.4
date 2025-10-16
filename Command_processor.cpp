#include "Command_processor.h"
#include "Param_helpers.h"
#include "EEPROM_Manager.h"

extern EEPROMManager eepromManager;
extern void showSystemStatus();

void Command_processor::handleSerialCommands() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    
    if (input.length() == 0) return;
    
    // Handle special commands
    if (input == "help") {
      showHelp();
      return;
    }
    
    if (input == "show") {
      showAllParameters();
      return;
    }
    
    if (input == "save") {
      eepromManager.saveConfig();
      Serial.println("Configuration saved");
      return;
    }
    
    if (input == "load_defaults") {
      eepromManager.resetToDefaults();
      Serial.println("Default Configuration loaded");
      return;
    }


    if (input == "status") {
      showSystemStatus();
      return;
    }
    
    // Universal parameter handler
    int spaceIndex = input.indexOf(' ');
    if (spaceIndex > 0) {
      String paramName = input.substring(0, spaceIndex);
      String valueStr = input.substring(spaceIndex + 1);
      
      // Find parameter by name with SERIAL_MENU flag
      for (int i = 0; i < PARAM_COUNT; i++) {
        if (paramName == system_params[i].name && 
            (system_params[i].flags & SERIAL_MENU)) {
          setParameter(system_params[i], valueStr);
          eepromManager.saveConfig(); // Save after successful change
          return;
        }
      }
      Serial.println("Error: Parameter not found or not accessible via serial");
    } else {
      Serial.println("Error: Use 'parameter value' format");
    }
  }
}

bool Command_processor::setParameter(ConfigParam& param, const String& value) {
  switch (param.type) {
    case TYPE_FLOAT: {
      float newValue = value.toFloat();
      if (newValue >= param.number.min_value && newValue <= param.number.max_value) {
        param.number.value = newValue;
        Serial.print(param.name);
        Serial.print(" set to: ");
        Serial.println(newValue);
        return true;
      }
      break;
    }
    
    case TYPE_UINT8: {
      int newValue = value.toInt();
      if (newValue >= param.uint8.min_value && newValue <= param.uint8.max_value) {
        param.uint8.value = newValue;
        Serial.print(param.name);
        Serial.print(" set to: ");
        Serial.println(newValue);
        return true;
      }
      break;
    }
    
    case TYPE_UINT16: {
      int newValue = value.toInt();
      if (newValue >= param.uint16.min_value && newValue <= param.uint16.max_value) {
        param.uint16.value = newValue;
        Serial.print(param.name);
        Serial.print(" set to: ");
        Serial.println(newValue);
        return true;
      }
      break;
    }
    
    case TYPE_INT16: {
        int newValue = value.toInt();
        if (newValue >= param.int16.min_value && newValue <= param.int16.max_value) {
            param.int16.value = newValue;
            Serial.print(param.name);
            Serial.print(" set to: ");
            Serial.println(newValue);
            return true;
        }
        break;
    }

    case TYPE_BOOL: {
      if (value == "1" || value == "true" || value == "on") {
        param.boolean.value = true;
        Serial.print(param.name);
        Serial.println(" set to: true");
        return true;
      } else if (value == "0" || value == "false" || value == "off") {
        param.boolean.value = false;
        Serial.print(param.name);
        Serial.println(" set to: false");
        return true;
      }
      break;
    }
    
    case TYPE_STRING: {
      if (value.length() < param.string.max_size) {
        strncpy(param.string.value, value.c_str(), param.string.max_size - 1);
        param.string.value[param.string.max_size - 1] = '\0';
        Serial.print(param.name);
        Serial.print(" set to: ");
        Serial.println(value);
        return true;
      }
      break;
    }
  }
  
  // If we get here, validation failed
  Serial.print("Error: ");
  Serial.print(param.name);
  Serial.print(" must be between ");
  
  switch (param.type) {
    case TYPE_FLOAT:
      Serial.print(param.number.min_value);
      Serial.print(" and ");
      Serial.println(param.number.max_value);
      break;
    case TYPE_UINT8:
      Serial.print(param.uint8.min_value);
      Serial.print(" and ");
      Serial.println(param.uint8.max_value);
      break;
    case TYPE_UINT16:
      Serial.print(param.uint16.min_value);
      Serial.print(" and ");
      Serial.println(param.uint16.max_value);
      break;
    case TYPE_INT16:
      Serial.print(param.int16.min_value);
      Serial.print(" and ");
      Serial.println(param.int16.max_value);
      break;
    case TYPE_STRING:
      Serial.print("string up to ");
      Serial.print(param.string.max_size - 1);
      Serial.println(" characters");
      break;
    case TYPE_BOOL:
      Serial.println("0/1, true/false, or on/off");
      break;
  }
  
  return false;
}

void Command_processor::showHelp() {
  Serial.println("Available commands:");
  for (int i = 0; i < PARAM_COUNT; i++) {
    if (system_params[i].flags & SERIAL_MENU) {
      Serial.print("  ");
      Serial.print(system_params[i].name);
      Serial.print(" (");
      Serial.print(typeToString(system_params[i].type));
      Serial.print(") - ");
      Serial.println(system_params[i].description);
    }
  }
  Serial.println("  help - Show this help");
  Serial.println("  show - Show all parameters");
  Serial.println("  save - Save configuration to EEPROM");
  Serial.println("  load_defaults - Reset configuration to defaults");
  Serial.println("  status - Show system status");
}

void Command_processor::showAllParameters() {
  Serial.println("All parameters:");
  for (int i = 0; i < PARAM_COUNT; i++) {
    ConfigParam& p = system_params[i];
    Serial.print("  ");
    Serial.print(p.name);
    Serial.print(" = ");
    
    if (p.flags & SECURED_VALUE) {
      Serial.print("******");
    } else {
      switch (p.type) {
        case TYPE_FLOAT:
          Serial.print(p.number.value);
          break;
        case TYPE_UINT8:
          Serial.print(p.uint8.value);
          break;
        case TYPE_UINT16:
          Serial.print(p.uint16.value);
          break;
        case TYPE_INT16:
          Serial.print(p.int16.value);
          break;
        case TYPE_BOOL:
          Serial.print(p.boolean.value ? "true" : "false");
          break;
        case TYPE_STRING:
          Serial.print("\"");
          Serial.print(p.string.value);
          Serial.print("\"");
          break;
      }
    }
    
    Serial.print(" (");
    Serial.print(p.description);
    Serial.println(")");
  }
}