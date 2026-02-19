#include "mbed.h"
#include "C12832.h"
#include "mbed2/299/platform/wait_api.h"

// =============================================================
// CLASSES (Your Potentiometer Logic)
// =============================================================
class Potentiometer {
private:
    AnalogIn inputSignal;
    float VDD, currentSampleNorm, currentSampleVolts;
public:
    Potentiometer(PinName pin, float v) : inputSignal(pin), VDD(v),
        currentSampleNorm(0.0f), currentSampleVolts(0.0f) {}

    void sample(void) {
        currentSampleNorm  = inputSignal.read();
        currentSampleVolts = currentSampleNorm * VDD;
    }
    const float getCurrentSampleNorm(void)  { return currentSampleNorm; }
    const float getCurrentSampleVolts(void) { return currentSampleVolts; }
};

class SamplingPotentiometer : public Potentiometer {
private:
    float samplingFrequency, samplingPeriod;
    Ticker sampler;
public:
    SamplingPotentiometer(PinName p, float v, float fs)
        : Potentiometer(p, v) {
        samplingFrequency = fs;
        samplingPeriod    = 1.0f / fs;
        sampler.attach(callback(this, &Potentiometer::sample), samplingPeriod);
    }
};

// =============================================================
// HARDWARE MAPPING
// =============================================================

// Motor 1 (Left)
DigitalOut  BIP1(PB_8);
DigitalOut  DIR1(PC_14);  
PwmOut      PWM1(PB_15);   

// Motor 2 (Right)
DigitalOut  BIP2(PB_9); 
DigitalOut  DIR2(PB_2); 
PwmOut      PWM2(PB_14);  

// Common & Shield
DigitalOut  ENA(PB_1);       
DigitalOut  heartbeat(LED1); 
C12832      lcd(D11, D13, D12, D7, D10);
InterruptIn userBtn(D4); 

// Encoders (CN7 Pins)
InterruptIn encLA(PC_10); 
DigitalIn   encLB(PC_12); 
InterruptIn encRA(PC_2); 
DigitalIn   encRB(PC_3);  

int main(void){
    wait_us(50);
}

