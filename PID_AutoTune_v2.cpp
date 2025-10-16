#include "PID_AutoTune_v2.h"
#include "BurstFireDimmer.h"

PID_AutoTune_v2::PID_AutoTune_v2(double* input, double* output, double* setpoint, BurstFireDimmer* dimmer) {
  _myInput = input;
  _myOutput = output;
  _mySetpoint = setpoint;
  _dimmer = dimmer;

  // Initialize with parameters from new system
  SetTunings(
    getParamFloat(PARAM_PID_KP),
    getParamFloat(PARAM_PID_KI), 
    getParamFloat(PARAM_PID_KD)
  );

  _sampleTime = getParamFloat(PARAM_PID_SAMPLE_TIME);
  _lastTime = millis() - _sampleTime;

  _autoTuneRunning = false;
  _autoTuneFinished = false;
  
  // Auto-tuning parameters
  _noiseBand = 1.0;
  _nLookback = 10;
  _lookbackSec = 60.0;
}

void PID_AutoTune_v2::Compute() {
  double currentTemp = *_myInput;
  double setpoint = *_mySetpoint;
  
  // SAFETY: Over-temperature protection
  if (currentTemp > setpoint + getPidMaxTempDiff() * 2) {
    // CRITICAL over-temperature: Force complete shutdown
    *_myOutput = 0;
    _dimmer->setPower(0);
    return;
  }
  
  // HEATING-ONLY LOGIC:
  if (currentTemp >= setpoint) {
    // AT or ABOVE setpoint: TURN OFF HEATING completely
    *_myOutput = 0;
    _ITerm = 0;  // Reset integral to prevent windup
  } 
  else {
    // BELOW setpoint: Apply heating based on how far below
    double tempDifference = setpoint - currentTemp;  // Positive value
    
    if (tempDifference > getPidSwitchingDelta()) {
      // Far below setpoint: Maximum allowed heating
      *_myOutput = ApplyPowerLimits(getPidMaxPower(), tempDifference);
    } else {
      // Close to setpoint: PID control
      double error = setpoint - currentTemp;
      _ITerm += (_Ki * error);
      _ITerm = constrain(_ITerm, 0, getPidMaxPower());  // Only positive
      double dInput = (currentTemp - _lastInput);
      *_myOutput = (_Kp * error) + _ITerm - (_Kd * dInput);
      *_myOutput = ApplyPowerLimits(*_myOutput, tempDifference);
    }
  }
  
  _dimmer->setPower(static_cast<uint8_t>(*_myOutput));
  _lastInput = currentTemp;
}

double PID_AutoTune_v2::ApplyPowerLimits(double output, double tempDifference) {
  double maxPower = getPidMaxPower();
  double minPower = getPidMinPower();
  double maxTempDiff = getPidMaxTempDiff();
  double minTempDiff = getPidMinTempDiff();
  
  // YOUR REQUESTED LOGIC:
  if (tempDifference >= maxTempDiff) {
    return maxPower;  // Use MAX power when difference >= MAX_TEMP_DIFF
  }
  else if (tempDifference <= minTempDiff) {
    return minPower;  // Use MIN power when difference <= MIN_TEMP_DIFF  
  }
  else {
    // Linear scaling between min and max
    double powerRange = maxPower - minPower;
    double tempRange = maxTempDiff - minTempDiff;
    double scale = (tempDifference - minTempDiff) / tempRange;
    return minPower + (powerRange * scale);
  }
}

void PID_AutoTune_v2::SetTunings(double Kp, double Ki, double Kd) {
  // Validate parameters
  if (Kp < 0 || Ki < 0 || Kd < 0) return;

  _dispKp = _Kp = Kp;
  _dispKi = _Ki = Ki;
  _dispKd = _Kd = Kd;

  // Adjust for sample time
  double SampleTimeInSec = ((double)_sampleTime) / 1000;
  _Ki *= SampleTimeInSec;
  _Kd /= SampleTimeInSec;
}

void PID_AutoTune_v2::StartAutoTune() {
  if (!_autoTuneRunning && !_autoTuneFinished) {
    _autoTuneRunning = true;
    //setPidAutotuneRunning(true);
    _setpointStart = *_mySetpoint;
    _absMax = *_myInput;
    _absMin = *_myInput;
    _oStep = 50; // Initial step 50%
    _peakCount = 0;
    _peakType = 0;
    _inputIndex = 0;
    _lastAutoTuneTime = millis();
    
    Serial.println("AutoTune started - will activate when close to setpoint");
  }
}

void PID_AutoTune_v2::StopAutoTune() {
  _autoTuneRunning = false;
  _autoTuneFinished = true;
  //setPidAutotuneRunning(false);
  Serial.println("AutoTune stopped");
}

bool PID_AutoTune_v2::IsAutoTuneFinished() {
  return _autoTuneFinished;
}

void PID_AutoTune_v2::AutoTuneRuntime() {
  if (!_autoTuneRunning) return;

  unsigned long now = millis();
  if ((now - _lastAutoTuneTime) < (_sampleTime)) return;
  _lastAutoTuneTime = now;

  double input = *_myInput;
  double currentDifference = abs(input - _setpointStart);

  // Only run auto-tune when close to setpoint
  if (currentDifference > getPidSwitchingDelta()) {
    // Too far from setpoint - wait until closer
    Serial.print("AutoTune waiting... difference: ");
    Serial.println(currentDifference);
    return;
  }

  double output;

  // Circular buffer for recent values
  _lastInputs[_inputIndex] = input;
  _inputIndex = (_inputIndex + 1) % _nLookback;

  // Find absolute max/min in lookback window
  _absMax = _lastInputs[0];
  _absMin = _lastInputs[0];
  for (int i = 1; i < _nLookback; i++) {
    _absMax = max(_absMax, _lastInputs[i]);
    _absMin = min(_absMin, _lastInputs[i]);
  }

  // Relay control logic for system excitation
  if (input > _setpointStart + _noiseBand) {
    output = 0; // Turn off
  } else if (input < _setpointStart - _noiseBand) {
    output = _oStep; // Turn on at oStep%
  } else {
    output = (*_myOutput > _oStep / 2) ? _oStep : 0; // Maintain current state
  }
  
  // Apply power limits
  output = ApplyPowerLimits(output, currentDifference);
  
  _dimmer->setPower(static_cast<uint8_t>(output));
  *_myOutput = output;

  // Peak detection logic
  if (input > _setpointStart + _noiseBand) {
    AutoTuneHelper(true);
  } else if (input < _setpointStart - _noiseBand) {
    AutoTuneHelper(false);
  }

  // Calculate coefficients when enough peaks detected
  if (_peakCount >= 6) {
    StopAutoTune();
    // Ziegler-Nichols method calculations
    double Ku = (4 * _oStep) / (3.14159 * (_absMax - _absMin) / 2);
    double Pu = (double)(_peakStartTime) / 1000.0;

    _Kp = 0.6 * Ku;
    _Ki = 1.2 * Ku / Pu;
    _Kd = 0.075 * Ku * Pu;

    // Adjust for sample time
    double SampleTimeInSec = ((double)_sampleTime) / 1000;
    _Ki *= SampleTimeInSec;
    _Kd /= SampleTimeInSec;

    // Store for display and save to config
    _dispKp = _Kp;
    _dispKi = _Ki;
    _dispKd = _Kd;
    
    // Save to parameter system
    setParamFloat(PARAM_PID_KP, _Kp);
    setParamFloat(PARAM_PID_KI, _Ki);
    setParamFloat(PARAM_PID_KD, _Kd);

    Serial.println("AutoTune Complete!");
    Serial.print("Kp: "); Serial.println(_Kp);
    Serial.print("Ki: "); Serial.println(_Ki);
    Serial.print("Kd: "); Serial.println(_Kd);
  }
}

void PID_AutoTune_v2::AutoTuneHelper(bool isMax) {
  // Detect peak type changes
  if (isMax != _peakType) {
    _peakType = isMax;
    _peakStartTime = millis();
    _peakValue = *_myInput;
  }

  // Count peaks and reset detection
  if ((millis() - _peakStartTime) > (_lookbackSec * 1000)) {
    _peakCount++;
    _peakType = !_peakType;
    _peakStartTime = millis();
    _peakValue = *_myInput;
  }
}