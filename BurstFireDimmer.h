#ifndef BurstFireDimmer_h
#define BurstFireDimmer_h

#include <Arduino.h>

class BurstFireDimmer {
  public:
    // Constructor: pin - output pin, freq - AC frequency (50 or 60Hz)
    BurstFireDimmer(uint8_t pin, uint8_t freq = 50);
    
    // Set power level: 0-100% (0 - off, 100 - full power)
    void setPower(uint8_t power);
    
    // Get current power level
    uint8_t getPower() const;
    
    // Initialize the dimmer - call in setup()
    void begin();
    
    // Zero-crossing interrupt handler - call this from your zero-cross ISR
    void handleZeroCross();
    
    // Configure filtering parameters (optional)
    void setFilterParameters(float window = 0.15, uint8_t expectedHz = 50);

    // Power change callback
    typedef void (*PowerChangeCallback)(uint8_t newPower);
    void setPowerChangeCallback(PowerChangeCallback callback);
    
  private:
    uint8_t _pin;
    uint8_t _frequency;
    volatile uint8_t _power;
    volatile uint16_t _halfWaveCount;
    volatile bool _enabled;
    
    // Filtering variables
    volatile unsigned long _lastZeroCrossTime;
    unsigned long _expectedPeriod;
    unsigned long _minPeriod;
    unsigned long _maxPeriod;
    float _filterWindow;

    // Callback pointer  // ← FIXED: Changed "/" to "//"
    PowerChangeCallback _powerCallback;
    
    // Calculate which half-waves should be ON
    bool shouldFire(uint16_t halfWaveIndex);
};

#endif