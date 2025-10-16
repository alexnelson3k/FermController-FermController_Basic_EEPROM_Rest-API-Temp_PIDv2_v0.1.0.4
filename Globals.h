#ifndef Globals_h
#define Globals_h

#include "Config.h"

#if MODULE_PID_AUTOTUNE == YES
  #include "PID_AutoTune_v2.h"
  extern PID_AutoTune_v2 pidController;
  extern double pidInput, pidOutput, pidSetpoint;
#endif

#if MODULE_ZERO_CROSS == YES
  #include "BurstFireDimmer.h"
  extern BurstFireDimmer heater;
#endif

extern ConfigSystem config;

#endif