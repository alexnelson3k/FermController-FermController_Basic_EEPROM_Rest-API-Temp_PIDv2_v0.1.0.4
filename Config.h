// Config.h
// ========
// Central configuration file with forward declarations
#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// Pin Configuration
// =================
#define ZERO_CROSS_PIN     5
#define TRIAC_PIN          6
#define TEMP_SENSOR_PIN    7    // GPIO pin for temperature sensor
#define ROTARY_CLK_PIN     10
#define ROTARY_DT_PIN      20
#define ROTARY_BTN_PIN     21
#define DISPLAY_SDA_PIN    8
#define DISPLAY_SCL_PIN    9

// System Configuration
// ====================
#define CONFIG_VERSION 1
#define FIRMWARE_VERSION "1.0.0"
#define SERIAL_BAUD_RATE 115200
#define ARDUINO_USB_CDC_ON_BOOT 1

#define WIFI_TIMEOUT 30000  // 30 seconds timeout

// HTTPS API Configuration
// ======================
#define HTTPS_PORT           443  // Standard HTTPS port
#define API_TIMEOUT         5000  // Request timeout in ms

// Reduce buffer sizes to save memory
#define HTTP_MAX_CLIENTS 2      // Reduce from default 4
#define HTTP_REQUEST_SIZE 1024  // Reduce from 2048
#define HTTP_RESPONSE_SIZE 2048 // Reduce from 4096

// SSL Certificate Configuration
// =============================
#define SSL_CERT_EXPIRY      365   // Certificate validity in days
#define SSL_KEY_SIZE         2048  // RSA key size (2048 or 4096)

// API Security Configuration
// ==========================
#define API_TOKEN_LENGTH     32    // Authentication token length
#define MAX_AUTH_ATTEMPTS    3     // Maximum failed login attempts
#define AUTH_TIMEOUT         30000 // Auth lockout timeout in ms

// Temperature Sensor Configuration
#define TEMP_SENSOR_TYPE       "DS18B20"  // or "NTC", "DHT22", etc.
#define TEMP_READ_INTERVAL     1000       // Read every 1 seconds
#define TEMP_CALIBRATION_OFFSET 0.0       // Calibration offset
#define TEMP_SMOOTHING_FACTOR   0.3       // Exponential smoothing factor

// Module Enable/Disable (YES/NO)
// ==============================
#define MODULE_ZERO_CROSS      YES
#define MODULE_HTTPS_API       YES
#define MODULE_TEMP_SENSOR     YES
#define MODULE_PID_AUTOTUNE    YES
#define MODULE_MQTT            NO
#define MODULE_INFLUXDB        NO
#define MODULE_POWER_MEASURE   NO
#define MODULE_I2C_SLAVE       NO
#define MODULE_BLUETOOTH       NO
#define MODULE_ROTARY_ENCODER  NO
#define MODULE_DISPLAY         NO

// Safety Limits
// =============
#define MAX_SAFE_TEMP 30.0
#define MIN_SAFE_TEMP 0.0

// Timing Constants
#define PID_COMPUTE_INTERVAL 1000

#define TEMP_SENSOR_PRECISION 12

#endif