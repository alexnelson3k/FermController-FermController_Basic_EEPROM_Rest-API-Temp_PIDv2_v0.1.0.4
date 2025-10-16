#include "Rotary_Module.h"
#include "Display_Module.h"
#include "Param_helpers.h"
#include "EEPROM_Manager.h"

extern ConfigParam system_params[PARAM_COUNT];
extern DisplayModule displayModule;
extern EEPROMManager eepromManager;

RotaryModule rotaryModule;

RotaryModule::RotaryModule() : lastClkState(HIGH) {}

bool RotaryModule::begin() {
    pinMode(ENCODER_CLK, INPUT_PULLUP);
    pinMode(ENCODER_DT, INPUT_PULLUP);
    pinMode(ENCODER_SW, INPUT_PULLUP);
    
    lastClkState = digitalRead(ENCODER_CLK);
    lastActivity = millis();
    
    currentParamIndex = findFirstDisplayParam();
    return true;
}

void RotaryModule::update() {
    handleRotation();
    handleButton();
    
    switch (currentMode) {
        case MODE_MAIN:
            handleMainMode();
            break;
        case MODE_PARAM_SCROLL_DISPLAY:    // ← ИЗМЕНИТЬ
            handleParamScrollDisplayMode(); // ← ИЗМЕНИТЬ
            break;
        case MODE_PARAM_SCROLL_EDIT:       // ← ИЗМЕНИТЬ  
            handleParamScrollEditMode();    // ← ИЗМЕНИТЬ
            break;
        case MODE_PARAM_EDIT:
            handleParamEditMode();
            break;
    }
}

void RotaryModule::handleRotation() {
    int clkState = digitalRead(ENCODER_CLK);
    
    if (clkState != lastClkState && (millis() - lastRotationTime) > DEBOUNCE_DELAY) {
        lastRotationTime = millis();
        
        if (clkState == HIGH) {
            int dtState = digitalRead(ENCODER_DT);
            int direction = (dtState != clkState) ? 1 : -1;
            
            lastActivity = millis();
            
            switch (currentMode) {
                case MODE_MAIN:
                    // Вращение из основного -> скролл DISPLAY_ACCESS
                    currentMode = MODE_PARAM_SCROLL_DISPLAY;
                    displayModule.setMode(PARAM_SCROLL);
                    currentParamIndex = findFirstDisplayParam();
                    displayModule.setSelectedParam(currentParamIndex);
                    Serial.println("MAIN -> DISPLAY_SCROLL");
                    break;
                    
                case MODE_PARAM_SCROLL_DISPLAY:
                    // Скролл параметров для просмотра
                    currentParamIndex = findNextDisplayParam(direction);
                    displayModule.setSelectedParam(currentParamIndex);
                    break;
                    
                case MODE_PARAM_SCROLL_EDIT:
                    // Скролл параметров для редактирования  
                    currentParamIndex = findNextRotaryParam(direction);
                    displayModule.setSelectedParam(currentParamIndex);
                    break;
                    
                case MODE_PARAM_EDIT:
                    // Редактирование значения
                    handleEditRotation(direction);
                    break;
            }
        }
    }
    lastClkState = clkState;
}

void RotaryModule::handleButton() {
    int btnState = digitalRead(ENCODER_SW);
    
    if (btnState == HIGH && !buttonPressed) {
        buttonPressed = true;
        pressStartTime = millis();
        buttonHandled = false;
    } 
    else if (btnState == LOW && buttonPressed) {
        unsigned long pressDuration = millis() - pressStartTime;
        buttonPressed = false;
        
        if (!buttonHandled) {
            lastActivity = millis();
            
            if (pressDuration < 1000) { // Короткое нажатие
                switch (currentMode) {
                    case MODE_MAIN:
                        // Короткое нажатие в основном -> ничего или показать подсказку
                        break;
                        
                    case MODE_PARAM_SCROLL_DISPLAY:
                        // Возврат в основной экран
                        currentMode = MODE_MAIN;
                        displayModule.setMode(MAIN_SCREEN);
                        Serial.println("DISPLAY_SCROLL -> MAIN (short press)");
                        break;
                        
                    case MODE_PARAM_SCROLL_EDIT:
                        // Вход в редактирование (если можно редактировать)
                        if (canEditCurrentParam()) {
                            currentMode = MODE_PARAM_EDIT;
                            displayModule.setMode(PARAM_EDIT);
                            Serial.println("EDIT_SCROLL -> EDIT");
                        }
                        break;
                        
                    case MODE_PARAM_EDIT:
                        // Сохранение и возврат в основной экран
                        saveCurrentValue();
                        currentMode = MODE_MAIN;
                        displayModule.setMode(MAIN_SCREEN);
                        Serial.println("EDIT -> MAIN (saved)");
                        break;
                }
            }
        }
    }
}

void RotaryModule::handleMainMode() {
    if (buttonPressed && !buttonHandled) {
        unsigned long pressDuration = millis() - pressStartTime;
        
        if (pressDuration >= 4000) { // Долгое нажатие 4с
            buttonHandled = true;
            currentMode = MODE_PARAM_SCROLL_EDIT;
            displayModule.setMode(PARAM_SCROLL);
            currentParamIndex = findFirstRotaryParam();
            displayModule.setSelectedParam(currentParamIndex);
            Serial.println("MAIN -> EDIT_SCROLL (long press)");
        }
    }
}

void RotaryModule::handleParamEditMode() {
    // Auto-return handled in shouldReturnToMain
}

int RotaryModule::findFirstDisplayParam() {
    for (int i = 0; i < PARAM_COUNT; i++) {
        if (system_params[i].flags & DISPLAY_ACCESS) {
            return i;
        }
    }
    return 0;
}

int RotaryModule::findFirstRotaryParam() {
    for (int i = 0; i < PARAM_COUNT; i++) {
        if (system_params[i].flags & DISPLAY_ACCESS && 
            system_params[i].flags & ROTARY_ACCESS) {
            return i;
        }
    }
    return findFirstDisplayParam();
}

int RotaryModule::findNextParam(int direction) {
    int startIndex = currentParamIndex;
    int newIndex = currentParamIndex;
    int attempts = 0;
    
    do {
        newIndex += direction;
        if (newIndex < 0) newIndex = PARAM_COUNT - 1;
        if (newIndex >= PARAM_COUNT) newIndex = 0;
        
        if (system_params[newIndex].flags & DISPLAY_ACCESS) {
            return newIndex;
        }
        
        attempts++;
    } while (newIndex != startIndex && attempts < PARAM_COUNT);
    
    return startIndex;
}

bool RotaryModule::canEditCurrentParam() {
    if (currentParamIndex >= 0 && currentParamIndex < PARAM_COUNT) {
        return (system_params[currentParamIndex].flags & ROTARY_ACCESS);
    }
    return false;
}

void RotaryModule::saveCurrentValue() {
    if (currentParamIndex >= 0 && currentParamIndex < PARAM_COUNT) {
        ParamIndex index = static_cast<ParamIndex>(currentParamIndex);
        ConfigParam& param = system_params[currentParamIndex];
        
        // Сохраняем в EEPROM если параметр не помечен как NO_FLASH_SAVE
        if (!(param.flags & NO_FLASH_SAVE)) {
            eepromManager.saveConfig();  // ← ВКЛЮЧИТЬ сохранение
            Serial.println("EEPROM saved!");
        } else {
            Serial.println("Parameter not saved to EEPROM (NO_FLASH_SAVE flag)");
        }
        
        Serial.printf("SAVED: %s = ", param.name);
        
        switch (param.type) {
            case TYPE_FLOAT:
                Serial.printf("%.1f\n", getParamFloat(index));
                break;
            case TYPE_UINT8:
                Serial.printf("%d\n", getParamUint8(index));
                break;
            case TYPE_UINT16:
                Serial.printf("%d\n", getParamUint16(index));
                break;
            case TYPE_BOOL:
                Serial.printf("%s\n", getParamBool(index) ? "ON" : "OFF");
                break;
            case TYPE_STRING:
                Serial.printf("%s\n", getParamString(index));
                break;
        }
    }
    
    displayModule.setSelectedParam(currentParamIndex);
}

bool RotaryModule::shouldReturnToMain() {
    if (currentMode != MODE_MAIN && (millis() - lastActivity > 10000)) {
        currentMode = MODE_MAIN;
        displayModule.setMode(MAIN_SCREEN);
        return true;
    }
    return false;
}

void RotaryModule::resetActivityTimer() {
    lastActivity = millis();
}

void RotaryModule::handleEditRotation(int direction) {
    if (currentParamIndex >= 0 && currentParamIndex < PARAM_COUNT) {
        ParamIndex index = static_cast<ParamIndex>(currentParamIndex);
        ConfigParam& param = system_params[currentParamIndex];
        
        //Serial.printf("EDIT: %s, type=%d, dir=%d, ", param.name, param.type, direction);
        
        // Флаг что значение изменилось
        static bool valueChanged = false;
        static int lastChangedParam = -1;
        
        switch (param.type) {
            case TYPE_FLOAT: {
                float oldValue = getParamFloat(index);
                float newFloat = oldValue + (direction * param.number.step);
                newFloat = constrain(newFloat, param.number.min_value, param.number.max_value);
                
                Serial.printf("FLOAT: old=%.1f, new=%.1f", oldValue, newFloat);
                
                if (newFloat != oldValue) {
                    setParamFloat(index, newFloat);
                    valueChanged = true;
                    lastChangedParam = currentParamIndex;
                    Serial.println(" - CHANGED");
                } else {
                    Serial.println(" - NO CHANGE");
                }
                break;
            }
            case TYPE_UINT8: {
                uint8_t oldValue = getParamUint8(index);
                uint8_t newUint8 = oldValue + (direction * param.uint8.step);
                newUint8 = constrain(newUint8, param.uint8.min_value, param.uint8.max_value);
                
                Serial.printf("UINT8: old=%d, new=%d", oldValue, newUint8);
                
                if (newUint8 != oldValue) {
                    setParamUint8(index, newUint8);
                    valueChanged = true;
                    lastChangedParam = currentParamIndex;
                    Serial.println(" - CHANGED");
                } else {
                    Serial.println(" - NO CHANGE");
                }
                break;
            }
            case TYPE_UINT16: {
                uint16_t oldValue = getParamUint16(index);
                uint16_t newUint16 = oldValue + (direction * param.uint16.step);
                newUint16 = constrain(newUint16, param.uint16.min_value, param.uint16.max_value);
                
                Serial.printf("UINT16: old=%d, new=%d", oldValue, newUint16);
                
                if (newUint16 != oldValue) {
                    setParamUint16(index, newUint16);
                    valueChanged = true;
                    lastChangedParam = currentParamIndex;
                    Serial.println(" - CHANGED");
                } else {
                    Serial.println(" - NO CHANGE");
                }
                break;
            }
            case TYPE_BOOL: {
                bool oldValue = getParamBool(index);
                Serial.printf("BOOL: old=%s", oldValue ? "ON" : "OFF");
                setParamBool(index, !oldValue);
                valueChanged = true;
                lastChangedParam = currentParamIndex;
                Serial.println(" - CHANGED");
                break;
            }
            default:
                Serial.printf("UNKNOWN TYPE: %d\n", param.type);
                break;
        }
        
        displayModule.setSelectedParam(currentParamIndex);
    }
}

int RotaryModule::findNextDisplayParam(int direction) {
    int startIndex = currentParamIndex;
    int newIndex = currentParamIndex;
    
    do {
        newIndex += direction;
        if (newIndex < 0) newIndex = PARAM_COUNT - 1;
        if (newIndex >= PARAM_COUNT) newIndex = 0;
        
        if (system_params[newIndex].flags & DISPLAY_ACCESS) {
            return newIndex;
        }
    } while (newIndex != startIndex);
    
    return startIndex;
}

int RotaryModule::findNextRotaryParam(int direction) {
    int startIndex = currentParamIndex;
    int newIndex = currentParamIndex;
    
    do {
        newIndex += direction;
        if (newIndex < 0) newIndex = PARAM_COUNT - 1;
        if (newIndex >= PARAM_COUNT) newIndex = 0;
        
        if (system_params[newIndex].flags & DISPLAY_ACCESS && 
            system_params[newIndex].flags & ROTARY_ACCESS) {
            return newIndex;
        }
    } while (newIndex != startIndex);
    
    return startIndex;
}

void RotaryModule::handleParamScrollDisplayMode() {
    // Автовозврат через 10 секунд бездействия
    if (millis() - lastActivity > 10000) {
        currentMode = MODE_MAIN;
        displayModule.setMode(MAIN_SCREEN);
        Serial.println("DISPLAY_SCROLL -> MAIN (timeout)");
        return;
    }
    
    // Долгое нажатие для перехода к редактированию
    if (buttonPressed && !buttonHandled) {
        unsigned long pressDuration = millis() - pressStartTime;
        
        if (pressDuration >= 4000) {
            buttonHandled = true;
            currentMode = MODE_PARAM_SCROLL_EDIT;
            currentParamIndex = findFirstRotaryParam();
            displayModule.setSelectedParam(currentParamIndex);
            Serial.println("DISPLAY_SCROLL -> EDIT_SCROLL (long press)");
        }
    }
    
    // Короткое нажатие обрабатывается в handleButton() и уже возвращает в MAIN
}

void RotaryModule::handleParamScrollEditMode() {
    if (buttonPressed && !buttonHandled) {
        unsigned long pressDuration = millis() - pressStartTime;
        
        if (pressDuration >= 4000) {
            buttonHandled = true;
            // Долгое нажатие в режиме выбора -> возврат к просмотру
            currentMode = MODE_PARAM_SCROLL_DISPLAY;
            currentParamIndex = findFirstDisplayParam();
            displayModule.setSelectedParam(currentParamIndex);
            Serial.println("EDIT_SCROLL -> DISPLAY_SCROLL (long press)");
        }
    }
}