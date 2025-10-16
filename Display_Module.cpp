#include "Display_Module.h"
#include "Param_helpers.h"
#include "Icons.h"
#include <WiFi.h>

extern ConfigParam system_params[PARAM_COUNT];
extern String getTimeHHMM();
DisplayModule displayModule;

DisplayModule::DisplayModule() : display(0x3C, DISPLAY_SDA_PIN, DISPLAY_SCL_PIN) {}

bool DisplayModule::begin() {
    display.init();
    display.setFont(ArialMT_Plain_10);
    display.flipScreenVertically();
    return true;
}

void DisplayModule::update() {
    display.clear();
    
    switch (currentMode) {
        case MAIN_SCREEN:
            drawMainScreen();
            break;
        case PARAM_SCROLL:
            drawParamScroll();
            break;
        case PARAM_EDIT:
            drawParamEdit(); // Всегда с рамкой
            break;
    }
    
    display.display();
}

void DisplayModule::drawMainScreen() {
    drawStatusBar();
    
    display.setFont(ArialMT_Plain_24);
    display.drawXbm(0, 18, 16, 16, temp_bits);
    display.drawString(16, 12, String(getParamFloat(PARAM_CURRENT_TEMP), 1));
    display.drawString(65, 12, String(getParamFloat(PARAM_TEMP_SETPOINT), 0) + "°C");
    
    display.drawXbm(0, 46, 16, 16, power_bits);
    display.drawString(18, 40, String(getParamUint8(PARAM_POWER_LEVEL)) + "%");
    display.drawString(80, 40, "PID");
}

void DisplayModule::drawParamScroll() {
    drawStatusBar();
    
    if (selectedParamIndex >= 0 && selectedParamIndex < PARAM_COUNT) {
        ConfigParam& param = system_params[selectedParamIndex];
        
        // Строка 2: имя параметра
        display.setFont(ArialMT_Plain_16);
        display.setTextAlignment(TEXT_ALIGN_LEFT);
        display.drawString(0, 14, param.name);
        
        // Строка 3: значение (обычный вид)
        display.setFont(ArialMT_Plain_24);
        String valueStr = getParamValueString(selectedParamIndex);
        display.drawString(0, 35, valueStr);
        
        // Индикатор выбора (точка слева) - ТОЛЬКО В РЕЖИМЕ СКРОЛЛА
        //display.fillCircle(2, 25, 2);
        
        //Serial.printf("DRAW SCROLL: %s = %s (normal)\n", param.name, valueStr.c_str());
    }
}

void DisplayModule::drawParamEdit() {
    drawStatusBar(); // Статус-бар рисуется нормально
    
    if (selectedParamIndex >= 0 && selectedParamIndex < PARAM_COUNT) {
        ConfigParam& param = system_params[selectedParamIndex];
        
        // ИНВЕРСИЯ ТОЛЬКО ОБЛАСТИ ЗНАЧЕНИЯ
        display.setColor(WHITE);
        display.fillRect(0, 35, 128, 24); // Белый фон
        display.setColor(BLACK);
        
        // Строка 2: имя параметра (над инверсной областью)
        display.setColor(WHITE); // Вернуть белый для текста
        display.setFont(ArialMT_Plain_16);
        display.drawString(0, 14, param.name);
        
        // Строка 3: значение на ИНВЕРСНОМ ФОНЕ
        display.setFont(ArialMT_Plain_24);
        String valueStr = getParamValueString(selectedParamIndex);
        
        // Черный текст на белом фоне
        display.setColor(BLACK);
        display.drawString(4, 35, valueStr);
        
        // Индикатор редактирования (тоже черный)
        display.drawString(110, 35, "◀▶");
        
        // Восстановить белый цвет для остального
        display.setColor(WHITE);
        
        //Serial.printf("DRAW EDIT: %s = %s (black on white)\n", param.name, valueStr.c_str());
    }
}

String DisplayModule::getParamValueString(int paramIndex) {
    // Использовать ParamIndex и хелперы
    ParamIndex index = static_cast<ParamIndex>(paramIndex);
    
    switch (system_params[paramIndex].type) {
        case TYPE_FLOAT:
            return String(getParamFloat(index), 1);
        case TYPE_UINT8:
            return String(getParamUint8(index));
        case TYPE_UINT16:
            return String(getParamUint16(index));
        case TYPE_INT16:
            return String(getParamInt16(index));
        case TYPE_BOOL:
            return getParamBool(index) ? "ON" : "OFF";
        case TYPE_STRING:
            return String(getParamString(index));
        default:
            return "?";
    }
}

void DisplayModule::drawStatusBar() {
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    
    bool wifiConnected = WiFi.status() == WL_CONNECTED;
    static bool wifiBlinkState = false;
    static unsigned long wifiBlinkTimer = 0;
    
    if (!wifiConnected) {
        if (millis() - wifiBlinkTimer > 500) {
            wifiBlinkState = !wifiBlinkState;
            wifiBlinkTimer = millis();
        }
        if (wifiBlinkState) {
            display.drawXbm(0, 0, 10, 10, wifi_icon_10x10);
        }
    } else {
        display.drawXbm(0, 0, 10, 10, wifi_icon_10x10);
        display.drawString(12, 0, String(WiFi.RSSI()));
    }
    
    String modeStr = "M";
    display.drawString(28, 0, modeStr);

    display.drawString(38, 0, getParamString(PARAM_UPTIME));
    
    display.drawString(100, 0, getTimeHHMM());
}

void DisplayModule::setMode(DisplayMode newMode) {
    currentMode = newMode;
    lastActivity = millis();
}

void DisplayModule::setSelectedParam(int paramIndex) {
    if (paramIndex >= 0 && paramIndex < PARAM_COUNT) {
        selectedParamIndex = paramIndex;
        lastActivity = millis();
    }
}