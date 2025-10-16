#ifndef TEMPERATURE_SENSOR_H
#define TEMPERATURE_SENSOR_H

#include <OneWire.h>
#include <DallasTemperature.h>
#include "param_helpers.h"

class Temperature_Sensor {
  private:
    OneWire oneWire;
    DallasTemperature sensors;
    DeviceAddress tempDeviceAddress;
    bool sensorFound = false;

  public:
    Temperature_Sensor(int pin);
    void begin();
    float readTemperature();
    bool isSensorConnected();
};

#endif