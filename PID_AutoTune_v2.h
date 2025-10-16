#ifndef PID_AutoTune_v2_h
#define PID_AutoTune_v2_h

#include <Arduino.h>
#include "Param_helpers.h"
#include "Param_types.h"

// Forward declaration
class BurstFireDimmer;

class PID_AutoTune_v2 {
  public:
    // Constructor with parameter system
    PID_AutoTune_v2(double* input, double* output, double* setpoint, BurstFireDimmer* dimmer);

    // Main PID computation method
    void Compute();
    
    // Manual tuning parameters setup
    void SetTunings(double Kp, double Ki, double Kd);
    
    // Auto-tuning methods
    void StartAutoTune();
    void StopAutoTune();
    bool IsAutoTuneFinished();
    void AutoTuneRuntime();

    // Get current tuning parameters
    double GetKp() const { return _dispKp; }
    double GetKi() const { return _dispKi; }
    double GetKd() const { return _dispKd; }

    // Apply power limits based on temperature difference
    double ApplyPowerLimits(double output, double tempDifference);

  private:
    // Pointers to external variables
    double* _myInput;
    double* _myOutput;
    double* _mySetpoint;
    BurstFireDimmer* _dimmer;

    // PID coefficients
    double _Kp, _Ki, _Kd;
    double _dispKp, _dispKi, _dispKd;

    // PID computation variables
    double _ITerm, _lastInput;
    unsigned long _lastTime;
    unsigned long _sampleTime;

    // Auto-tuning variables
    bool _autoTuneRunning;
    bool _autoTuneFinished;
    unsigned long _peakStartTime;
    double _peakValue;
    int _peakType;
    int _peakCount;
    double _lastInputs[100];
    int _inputIndex;
    float _noiseBand;
    int _nLookback;
    float _lookbackSec;
    double _absMax, _absMin;
    double _setpointStart;
    double _oStep;
    int _lastAutoTuneTime;

    // Helper method for peak detection
    void AutoTuneHelper(bool isMax);
    
    // Helper methods to get parameters from new system
    double getPidMaxPower() { return getParamFloat(PARAM_PID_MAX_POWER); }
    double getPidMinPower() { return getParamFloat(PARAM_PID_MIN_POWER); }
    double getPidMaxTempDiff() { return getParamFloat(PARAM_PID_MAX_TEMP_DIFF); }
    double getPidMinTempDiff() { return getParamFloat(PARAM_PID_MIN_TEMP_DIFF); }
    double getPidSwitchingDelta() { return getParamFloat(PARAM_PID_SWITCHING_DELTA); }
    //bool getPidAutotuneRunning() { return getParamBool(PARAM_PID_AUTOTUNE_RUNNING); }
    //void setPidAutotuneRunning(bool value) { setParamBool(PARAM_PID_AUTOTUNE_RUNNING, value); }
};

#endif