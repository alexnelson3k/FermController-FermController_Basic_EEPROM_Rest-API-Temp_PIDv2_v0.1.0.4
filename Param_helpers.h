#ifndef PARAM_HELPERS_H
#define PARAM_HELPERS_H

#include "Param_types.h"

// Value getters/setters
float getParamFloat(ParamIndex index);
void setParamFloat(ParamIndex index, float value);
uint8_t getParamUint8(ParamIndex index); 
void setParamUint8(ParamIndex index, uint8_t value);
uint16_t getParamUint16(ParamIndex index);
void setParamUint16(ParamIndex index, uint16_t value);
int16_t getParamInt16(ParamIndex index);
void setParamInt16(ParamIndex index, int16_t value);
bool getParamBool(ParamIndex index);
void setParamBool(ParamIndex index, bool value);
const char* getParamString(ParamIndex index);
void setParamString(ParamIndex index, const char* value);

// Display helper
String getParamDisplayValue(ParamIndex index);

// Type conversion
const char* typeToString(ParamType type);

#endif