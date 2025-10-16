#include "BurstFireDimmer.h"

BurstFireDimmer::BurstFireDimmer(uint8_t pin, uint8_t freq) {
  _pin = pin;
  _frequency = freq;
  _power = 0;
  _halfWaveCount = 0;
  _enabled = false;
  _lastZeroCrossTime = 0;
  _filterWindow = 0.15;
  _expectedPeriod = 1000000UL / (_frequency * 2);
  _minPeriod = _expectedPeriod * (1.0 - _filterWindow);
  _maxPeriod = _expectedPeriod * (1.0 + _filterWindow);

  // Initialize callback to nullptr
  _powerCallback = nullptr;
}

// Set power change callback
void BurstFireDimmer::setPowerChangeCallback(PowerChangeCallback callback) {
  _powerCallback = callback;
}

void BurstFireDimmer::setPower(uint8_t power) {
  noInterrupts();
  uint8_t oldPower = _power;  // ← THIS IS THE CRITICAL FIX!
  _power = (power > 100) ? 100 : power;
  _halfWaveCount = 0;
  interrupts();

  if (_powerCallback && oldPower != _power) {
    _powerCallback(_power);
  }
}

// Get current power
uint8_t BurstFireDimmer::getPower() const {
  return _power;
}

void BurstFireDimmer::begin() {
  pinMode(_pin, OUTPUT);
  digitalWrite(_pin, LOW);
}

void BurstFireDimmer::setFilterParameters(float window, uint8_t expectedHz) {
  _filterWindow = window;
  _frequency = expectedHz;
  _expectedPeriod = 1000000UL / (_frequency * 2);
  _minPeriod = _expectedPeriod * (1.0 - _filterWindow);
  _maxPeriod = _expectedPeriod * (1.0 + _filterWindow);
}

void BurstFireDimmer::handleZeroCross() {
  unsigned long currentTime = micros();
  unsigned long timeSinceLast = currentTime - _lastZeroCrossTime;
  
  if (timeSinceLast > _minPeriod && timeSinceLast < _maxPeriod) {
    // Valid zero-crossing
    if (_power == 0) {
      digitalWrite(_pin, LOW);
    } else if (_power == 100) {
      digitalWrite(_pin, HIGH);
    } else {
      bool fire = shouldFire(_halfWaveCount);
      digitalWrite(_pin, fire ? HIGH : LOW);
    }
    
    _halfWaveCount++;
    if (_halfWaveCount >= 100) _halfWaveCount = 0;
    _lastZeroCrossTime = currentTime;
  }
  else if (timeSinceLast < _minPeriod) {
    // Noise - ignore
    return;
  }
  else {
    // Missed zero - count it
    if (_power == 0) {
      digitalWrite(_pin, LOW);
    } else if (_power == 100) {
      digitalWrite(_pin, HIGH);
    } else {
      bool fire = shouldFire(_halfWaveCount);
      digitalWrite(_pin, fire ? HIGH : LOW);
    }
    
    _halfWaveCount++;
    if (_halfWaveCount >= 100) _halfWaveCount = 0;
    _lastZeroCrossTime = currentTime;
  }
}

// FIXED shouldFire method:
bool BurstFireDimmer::shouldFire(uint16_t halfWaveIndex) {
  // Bresenham-like algorithm for perfect distribution
  static int16_t error = 0;
  
  if (_power == 0) return false;
  if (_power == 100) return true;
  
  error += _power;
  if (error >= 100) {
    error -= 100;
    return true;
  }
  return false;
}