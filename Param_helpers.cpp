#include "Param_helpers.h"

// Getters
float getParamFloat(ParamIndex index) {
    return system_params[index].number.value;
}

uint8_t getParamUint8(ParamIndex index) {
    return system_params[index].uint8.value;
}

uint16_t getParamUint16(ParamIndex index) {
    return system_params[index].uint16.value;
}

int16_t getParamInt16(ParamIndex index) {
    return system_params[index].int16.value;
}

bool getParamBool(ParamIndex index) {
    return system_params[index].boolean.value;
}

const char* getParamString(ParamIndex index) {
    return system_params[index].string.value;
}

// Setters  
void setParamFloat(ParamIndex index, float value) {
    system_params[index].number.value = value;
}

void setParamUint8(ParamIndex index, uint8_t value) {
    system_params[index].uint8.value = value;
}

void setParamUint16(ParamIndex index, uint16_t value) {
    system_params[index].uint16.value = value;
}

void setParamInt16(ParamIndex index, int16_t value) {
    system_params[index].int16.value = value;
}

void setParamBool(ParamIndex index, bool value) {
    system_params[index].boolean.value = value;
}

void setParamString(ParamIndex index, const char* value) {
    strncpy(system_params[index].string.value, value, system_params[index].string.max_size - 1);
    system_params[index].string.value[system_params[index].string.max_size - 1] = '\0';
}

// Display helper
String getParamDisplayValue(ParamIndex index) {
    ConfigParam& param = system_params[index];
    
    // Mask secured values
    if (param.flags & SECURED_VALUE) {
        return "******";
    }
    
    // Return actual value for non-secured parameters
    switch (param.type) {
        case TYPE_FLOAT:
            return String(param.number.value);  // ← ПРЯМОЙ ДОСТУП к значению
        case TYPE_UINT8:
            return String(param.uint8.value);   // ← ПРЯМОЙ ДОСТУП к значению
        case TYPE_UINT16:
            return String(param.uint16.value);  // ← ПРЯМОЙ ДОСТУП к значению
        case TYPE_INT16:
            return String(param.int16.value);  // ← ПРЯМОЙ ДОСТУП к значению
        case TYPE_BOOL:
            return param.boolean.value ? "true" : "false";  // ← ПРЯМОЙ ДОСТУП
        case TYPE_STRING:
            return String(param.string.value);  // ← ПРЯМОЙ ДОСТУП к значению
        default:
            return "unknown";
    }
}

// Type conversion
const char* typeToString(ParamType type) {
    switch (type) {
        case TYPE_FLOAT: return "float";
        case TYPE_UINT8: return "uint8";
        case TYPE_UINT16: return "uint16";
        case TYPE_INT16: return "uint16";
        case TYPE_BOOL: return "bool";
        case TYPE_STRING: return "string";
        default: return "unknown";
    }
}