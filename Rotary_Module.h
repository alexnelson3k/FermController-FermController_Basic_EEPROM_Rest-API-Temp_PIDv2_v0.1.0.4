#ifndef ROTARY_MODULE_H
#define ROTARY_MODULE_H

#include "Param_types.h"
#include "Config.h"  // Добавляем для доступа к пинам
#include "EEPROM_Manager.h"

class RotaryModule {
  public:
    RotaryModule();
    bool begin();
    void update();
    // Public for access from main
    bool shouldReturnToMain();
    void resetActivityTimer();
    
  private:
    int fastRotationCount = 0;
    unsigned long lastFastRotation = 0;
    static const unsigned long FAST_ROTATION_TIMEOUT = 200; // ms
    static const int FAST_ROTATION_THRESHOLD = 3; // щелчков для ускорения
    unsigned long lastRotationTime = 0;
    static const unsigned long DEBOUNCE_DELAY = 1; // ms
    // Encoder pins from Config.h
    static const int ENCODER_CLK = ROTARY_CLK_PIN;
    static const int ENCODER_DT = ROTARY_DT_PIN;
    static const int ENCODER_SW = ROTARY_BTN_PIN;
    
    // State variables
    int lastClkState;
    int currentParamIndex = 0;
    unsigned long pressStartTime = 0;
    bool buttonPressed = false;
    bool buttonHandled = false;
    unsigned long lastActivity = 0;
    
    // Mode tracking
    enum SystemMode {
        MODE_MAIN,
        MODE_PARAM_SCROLL_DISPLAY,
        MODE_PARAM_SCROLL_EDIT,
        MODE_PARAM_EDIT

    } currentMode = MODE_MAIN;
    
    // Encoder methods
    void handleRotation();
    void handleButton();
    void handleMainMode();
    void handleParamScrollDisplayMode();
    void handleParamScrollEditMode();
    void handleParamEditMode();
    void handleEditRotation(int direction);
    

    // Helper methods
    int findFirstDisplayParam();
    int findFirstRotaryParam();
    int findNextDisplayParam(int direction);
    int findNextRotaryParam(int direction);
    int findNextParam(int direction);
    bool canEditCurrentParam();
    void saveCurrentValue();
};

#endif