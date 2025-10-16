#ifndef PARAM_TYPES_H
#define PARAM_TYPES_H

#include <Arduino.h>

enum ParamType {
    TYPE_FLOAT,
    TYPE_UINT8,
    TYPE_UINT16,
    TYPE_INT16, 
    TYPE_BOOL,
    TYPE_STRING
};

enum ParamFlags {
    SERIAL_MENU     = 0x01,  // Included in serial menu
    DISPLAY_ACCESS  = 0x02,  // Included in display menu
    ROTARY_ACCESS   = 0x04,  // Included in rotary encoder menu
    API_ACCESS      = 0x08,  // Included in API get/post
    MQTT_ACCESS     = 0x10,  // Included in MQTT
    INFLUXDB_REPORT = 0x20,  // Included in InfluxDB reporting
    I2C_ACCESS      = 0x40,  // Included in I2C
    BLUETOOTH_ACCESS= 0x80,  // Included in Bluetooth
    NO_FLASH_SAVE   = 0x100, // Don't save to flash (RAM only)
    SECURED_VALUE   = 0x200, // Mask value when displaying (for passwords)
};

struct ConfigParam {
    const char* name;
    const char* description;
    ParamType type;
    uint16_t flags;
    
    union {
        struct {
            float value;
            float min_value;
            float max_value;
            float step;
            float default_value;
        } number;
        
        struct {
            uint8_t value;
            uint8_t min_value;
            uint8_t max_value;
            uint8_t step;
            uint8_t default_value;
        } uint8;
        
        struct {
            uint16_t value;
            uint16_t min_value;
            uint16_t max_value;
            uint16_t step;
            uint16_t default_value;
        } uint16;

        struct {
            int16_t value;
            int16_t min_value;
            int16_t max_value;
            int16_t step;
            int16_t default_value;
        } int16;
        
        struct {
            bool value;
            bool default_value;
        } boolean;
        
        struct {
            char value[64];
            size_t max_size;
            const char* default_value;
        } string;
    };
};

// Parameter indices
enum ParamIndex {
    // System
    PARAM_VERSION,
    PARAM_UPTIME,
    PARAM_POWER_LEVEL,
	
    // Temperature
    PARAM_TEMP_SETPOINT,
    PARAM_TEMP_SETPOINT_MIN,
    PARAM_TEMP_SETPOINT_MAX,
    PARAM_TEMP_HYSTERESIS,
    PARAM_CURRENT_TEMP,
    PARAM_TEMP_CALIBRATION,
    PARAM_TEMP_SENSOR_TYPE,
    PARAM_UPDATE_INTERVAL,
    PARAM_HEATER_ENABLED,
	
    // PID
    PARAM_PID_KP,
    PARAM_PID_KI,
    PARAM_PID_KD,
    PARAM_PID_SAMPLE_TIME,
    PARAM_PID_MAX_POWER,
    PARAM_PID_MIN_POWER,
    PARAM_PID_MAX_TEMP_DIFF,
    PARAM_PID_MIN_TEMP_DIFF,
    PARAM_PID_SWITCHING_DELTA,
    PARAM_HEATER_RUNNING,
	PARAM_OPERATING_MODE,
    
    // Network
    PARAM_WIFI_SSID,
    PARAM_WIFI_PASSWORD,
    PARAM_MQTT_SERVER,
    PARAM_MQTT_PORT,
    PARAM_API_TOKEN,
    PARAM_HTTPS_ENABLED,
    PARAM_HTTPS_PORT,
    
    // NTP Configuration
    PARAM_NTP_SERVER,
    PARAM_NTP_GMT_OFFSET, 
    PARAM_NTP_DAYLIGHT_OFFSET,

    PARAM_COUNT
};

extern ConfigParam system_params[PARAM_COUNT];

// Helper functions
float getParamFloat(ParamIndex index);
void setParamFloat(ParamIndex index, float value);
uint8_t getParamUint8(ParamIndex index);
void setParamUint8(ParamIndex index, uint8_t value);
bool getParamBool(ParamIndex index);
void setParamBool(ParamIndex index, bool value);
const char* getParamString(ParamIndex index);
void setParamString(ParamIndex index, const char* value);

// Security helper
String getParamDisplayValue(ParamIndex index);

#endif