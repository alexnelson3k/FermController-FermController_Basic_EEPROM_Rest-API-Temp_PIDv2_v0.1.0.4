#include "Temperature_Sensor.h"
#include "config.h"

Temperature_Sensor::Temperature_Sensor(int pin) : oneWire(pin), sensors(&oneWire) {}

void Temperature_Sensor::begin() {
  sensors.begin();
  if (sensors.getDeviceCount() == 0) {
    Serial.println("No temperature sensors found!");
    sensorFound = false;
    return;
  }
  
  if (!sensors.getAddress(tempDeviceAddress, 0)) {
    Serial.println("Unable to find address for Temperature sensor"); 
    sensorFound = false;
    return;
  }
  
  sensors.setResolution(tempDeviceAddress, TEMP_SENSOR_PRECISION);
  sensorFound = true;
  Serial.println("Temperature Sensor initialized");
}

float Temperature_Sensor::readTemperature() {
  if (!sensorFound) {
    return -127.0;
  }
  
  sensors.requestTemperatures();
  float tempC = sensors.getTempC(tempDeviceAddress);
  
  if (tempC == DEVICE_DISCONNECTED_C) {
    Serial.println("Error: Could not read temperature data");
    return -127.0;
  }
  
  return tempC;
}

bool Temperature_Sensor::isSensorConnected() {
  return sensorFound;
}