#include <Arduino.h>
#include <EEPROM.h>
#include <WiFi.h>
#include "time.h"
#include "Display_Module.h"
#include "Rotary_Module.h"
#include <algorithm>
using std::min;

// Configuration
#include "config.h"

// New parameter system
#include "Param_types.h"
//#include "param_config.cpp"
//#include "Param_helpers.cpp"

// Communication
#include "Command_processor.h"
#include "HTTPS_Module.h"

// Controllers
#include "PID_AutoTune_v2.h"
#include "Temperature_Sensor.h"
#include "BurstFireDimmer.h"

// Storage
#include "EEPROM_Manager.h"

double currentTemp = 0;
double setpoint = 0;
double pidOutput = 0;

extern DisplayModule displayModule;
extern RotaryModule rotaryModule;

// Instances
Command_processor cmdProcessor;
EEPROMManager eepromManager;
BurstFireDimmer dimmer(ZERO_CROSS_PIN, TRIAC_PIN);
PID_AutoTune_v2 pidController(&currentTemp, &pidOutput, &setpoint, &dimmer);
Temperature_Sensor tempSensor(TEMP_SENSOR_PIN);


// Global variables
unsigned long lastTempRead = 0;
unsigned long lastPIDCompute = 0;
unsigned long lastConfigSave = 0;
unsigned long lastStatusPublish = 0;
unsigned long previousTempMillis = 0;
unsigned long previousPIDMillis = 0;
unsigned long previousSaveMillis = 0;

void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    Serial.println();
    Serial.println("=== Fermenter Controller Starting ===");

    //Setup NTP
    setupNTP();
    
    // Initialize EEPROM and load configuration
    eepromManager.begin();
    if (!eepromManager.loadConfig()) {
        Serial.println("No saved config found, using defaults");
        eepromManager.resetToDefaults();
    } else {
        Serial.println("Configuration loaded from EEPROM");
    }
    
    // Принудительно установить heater_enabled если он в RAM
    setParamBool(PARAM_HEATER_ENABLED, true);

    // Initialize hardware
    tempSensor.begin();  
    dimmer.begin();     
    
    // Initialize modules
    displayModule.begin();
    rotaryModule.begin();

    // Connect to WiFi BEFORE initializing modules
    if (!connectToWiFi()) {
    Serial.println("WiFi connection failed. Continuing without network...");
    // You can choose to continue or halt here
  }

  // Initialize HTTPS API module
  initHTTPSModule();

    // Initialize MQTT (если есть)
    // setupMQTT(); ← ЗАКОММЕНТИРОВАТЬ если вызывает ошибки
    
    Serial.println("=== Fermenter Controller Ready ===");
    cmdProcessor.showHelp();
}

void loop() {
    unsigned long currentMillis = millis();

    // Do every second: Read sensors, Update uptime
    if (currentMillis - previousTempMillis >= getParamFloat(PARAM_UPDATE_INTERVAL)) {
        setParamString(PARAM_UPTIME, getUptime());
        updateHeaterStatus();
        float temperatureReading = tempSensor.readTemperature();
        if (temperatureReading != -127.0) {
            setParamFloat(PARAM_CURRENT_TEMP, temperatureReading);
        }
        previousTempMillis = currentMillis;
    }

    // PID computation and power control
    if (currentMillis - previousPIDMillis >= getParamFloat(PARAM_PID_SAMPLE_TIME)) {
        float currentTemp = getParamFloat(PARAM_CURRENT_TEMP);
        float setpoint = getParamFloat(PARAM_TEMP_SETPOINT);
        
        if (getParamUint8(PARAM_OPERATING_MODE) == 1) { // Auto mode
            // Check if we should use PID or full power
            if (fabs(setpoint - currentTemp) > getParamFloat(PARAM_PID_SWITCHING_DELTA)) {
                // Outside PID range - use full power or off
                if (currentTemp < setpoint) {
                    setParamUint8(PARAM_POWER_LEVEL, (uint8_t)getParamFloat(PARAM_PID_MAX_POWER));
                } else {
                    setParamUint8(PARAM_POWER_LEVEL, 0);
                }
            } else {
                // Within PID range - use simple power calculation based on temperature difference
                float tempDifference = setpoint - currentTemp;
                float maxPower = getParamFloat(PARAM_PID_MAX_POWER);
                float minPower = getParamFloat(PARAM_PID_MIN_POWER);
                float maxTempDiff = getParamFloat(PARAM_PID_MAX_TEMP_DIFF);
                float minTempDiff = getParamFloat(PARAM_PID_MIN_TEMP_DIFF);
                
                if (tempDifference >= maxTempDiff) {
                    setParamUint8(PARAM_POWER_LEVEL, (uint8_t)maxPower);
                } else if (tempDifference <= minTempDiff) {
                    setParamUint8(PARAM_POWER_LEVEL, (uint8_t)minPower);
                } else {
                    // Linear scaling between min and max
                    float powerRange = maxPower - minPower;
                    float tempRange = maxTempDiff - minTempDiff;
                    float scale = (tempDifference - minTempDiff) / tempRange;
                    float calculatedPower = minPower + (powerRange * scale);
                    setParamUint8(PARAM_POWER_LEVEL, (uint8_t)calculatedPower);
                }
            }
        }
        // In manual mode, powerLevel is set directly by user via commands
        
        previousPIDMillis = currentMillis;
    }

    // Apply power to heater (with safety check)
    if (getParamBool(PARAM_HEATER_ENABLED)) {
        dimmer.setPower(getParamUint8(PARAM_POWER_LEVEL));
    } else {
        dimmer.setPower(0);
        setParamUint8(PARAM_POWER_LEVEL, 0);
    }

    // Process serial commands
    cmdProcessor.handleSerialCommands();

    // Handle web server
    //httpsModule.update();
    httpsModule.handleClient();

    rotaryModule.update();
    displayModule.update();

    // Safety checks
    //float currentTemp = getParamFloat(PARAM_CURRENT_TEMP);
    //if (currentTemp > MAX_SAFE_TEMP) {
    //    Serial.println("SAFETY: Over-temperature protection activated!");
    //    setParamBool(PARAM_HEATER_ENABLED, false);
    //    setParamUint8(PARAM_POWER_LEVEL, 0);
    //}

    //delay(10);
}


void showSystemStatus() {
    Serial.println("=== System Status ===");
    Serial.print("Mode: ");
    Serial.println(getParamUint8(PARAM_OPERATING_MODE) == 1 ? "AUTO" : "MANUAL");
    Serial.print("Temperature: ");
    Serial.print(getParamFloat(PARAM_CURRENT_TEMP));
    Serial.println(" °C");
    Serial.print("Setpoint: ");
    Serial.print(getParamFloat(PARAM_TEMP_SETPOINT));
    Serial.println(" °C");
    Serial.print("Power Level: ");
    Serial.print(getParamUint8(PARAM_POWER_LEVEL));
    Serial.println(" %");
    Serial.print("Heater: ");
    Serial.println(getParamBool(PARAM_HEATER_ENABLED) ? "ENABLED" : "DISABLED");
    Serial.print("Heater Status: ");
    Serial.println(getParamBool(PARAM_HEATER_RUNNING) ? "RUNNING" : "STOPPED");
    Serial.println("===================");
}

void checkSafetyLimits() {
    float currentTemp = getParamFloat(PARAM_CURRENT_TEMP);
    
    // Over-temperature protection
    if (currentTemp > MAX_SAFE_TEMP) {
        Serial.println("SAFETY: Over-temperature detected!");
        setParamBool(PARAM_HEATER_ENABLED, false);
        setParamUint8(PARAM_POWER_LEVEL, 0);
    }
    
    // Under-temperature protection  
    if (currentTemp < MIN_SAFE_TEMP) {
        Serial.println("SAFETY: Under-temperature detected!");
        // Could implement additional safety actions here
    }
}

// WiFi setup function (from your existing code)
void setupWiFi() {
    // Your existing WiFi setup code
    // Now using parameters from new system:
    // getParamString(PARAM_WIFI_SSID)
    // getParamString(PARAM_WIFI_PASSWORD)
}

bool connectToWiFi() {
  Serial.println("Connecting to WiFi...");
  
  if (strlen(getParamString(PARAM_WIFI_SSID)) == 0) {
    Serial.println("No WiFi credentials configured.");
    return false;
  }
  
  WiFi.begin(getParamString(PARAM_WIFI_SSID), getParamString(PARAM_WIFI_PASSWORD));
  
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < WIFI_TIMEOUT) {
    delay(500);
    Serial.print(".");
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    return true;
  } else {
    Serial.println("\nWiFi connection failed!");
    return false;
  }
}

void initHTTPSModule() {
  if (httpsModule.begin()) {
    Serial.println("HTTPS API module: Initialized");
  } else {
    Serial.println("HTTPS API module: Failed to initialize");
  }
}
// MQTT setup function (from your existing code)
void setupMQTT() {
    // Your existing MQTT setup code
    // Now using parameters from new system:
    // getParamString(PARAM_MQTT_SERVER)
    // getParamUint16(PARAM_MQTT_PORT)
}

void publishMQTTStatus() {
    // Your existing MQTT publishing code
    // Now using parameters from new system
    // Only publish parameters with MQTT_ACCESS flag
}

void updateHeaterStatus() {
    uint8_t powerLevel = getParamUint8(PARAM_POWER_LEVEL);
    bool heaterRunning = (powerLevel > 0);
    
    bool currentStatus = getParamBool(PARAM_HEATER_RUNNING);
    if (currentStatus != heaterRunning) {
        setParamBool(PARAM_HEATER_RUNNING, heaterRunning);
        Serial.printf("Heater %s (power=%d%%)\n", 
                     heaterRunning ? "STARTED" : "STOPPED", powerLevel);
    }
}

void setupNTP() {
    const char* ntpServer = getParamString(PARAM_NTP_SERVER);
    long gmtOffset_sec = (long)getParamInt16(PARAM_NTP_GMT_OFFSET) * 3600;
    int daylightOffset_sec = (int)getParamInt16(PARAM_NTP_DAYLIGHT_OFFSET) * 3600;
    
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    Serial.printf("NTP: %s, GMT: %dh, DST: %dh\n", 
                  ntpServer, 
                  getParamInt16(PARAM_NTP_GMT_OFFSET),
                  getParamInt16(PARAM_NTP_DAYLIGHT_OFFSET));
}

const char* getUptime() {
  static char buffer[12]; // static чтобы память не очищалась
  unsigned long ms = millis();
  unsigned long seconds = ms / 1000;
  unsigned long days = seconds / 86400;
  seconds %= 86400;
  unsigned long hours = seconds / 3600;
  seconds %= 3600;
  unsigned long minutes = seconds / 60;
  seconds %= 60;

  snprintf(buffer, sizeof(buffer), "%02lu:%02lu:%02lu:%02lu", days, hours, minutes, seconds);
  return buffer;
}

String getTimeHHMM() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "00:00"; // fallback if time not available
  }
  char buffer[6];
  strftime(buffer, sizeof(buffer), "%H:%M", &timeinfo);
  return String(buffer);
}