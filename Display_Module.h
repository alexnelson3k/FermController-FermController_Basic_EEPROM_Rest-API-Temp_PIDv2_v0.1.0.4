#ifndef DISPLAY_MODULE_H
#define DISPLAY_MODULE_H


#include <SH1106Wire.h>
#include "Config.h"
#include "Param_types.h"


enum DisplayMode {
    MAIN_SCREEN,
    PARAM_SCROLL, 
    PARAM_EDIT
};

class DisplayModule {
  public:
    DisplayModule();
    bool begin();
    void update();
    void setMode(DisplayMode newMode);
    void setSelectedParam(int paramIndex);
    void setEditValue(float newValue);
    
  private:
    // Display pins from Config.h
    SH1106Wire display;
    DisplayMode currentMode = MAIN_SCREEN;
    int selectedParamIndex = 0;
    unsigned long lastActivity = 0;
    
    void drawMainScreen();
    void drawParamScroll();
    void drawParamEdit();
    void drawStatusBar();
    String getParamValueString(int paramIndex);
    void drawScrollIndicator(int totalParams, int currentIndex);
};

#endif