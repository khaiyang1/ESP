#include "mbed.h"
#include "C12832.h"

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

// Motor 1 (Left)
DigitalOut  BIP1(PA_13);
DigitalOut  DIR1(PC_14);  
PwmOut      PWM1(PB_15);   

// Motor 2 (Right)
DigitalOut  BIP2(PA_14); 
DigitalOut  DIR2(PB_2); 
PwmOut      PWM2(PB_14);  

// Common & Shield
DigitalOut  ENA(PB_1);       
DigitalOut  heartbeat(LED1); 
C12832      lcd(D11, D13, D12, D7, D10);

// Encoders (CN7 Pins)
InterruptIn encLA(PC_10); 
DigitalIn   encLB(PC_12); 
InterruptIn encRA(PC_2); 
DigitalIn   encRB(PC_3);  

int main(void) {
    // 1. Initialize PWM Period
    PWM1.period_us(50); // 20kHz
    PWM2.period_us(50);

    // 2. Initialize Potentiometers
    SamplingPotentiometer potLeft(PA_0, 3.3f, 100);
    SamplingPotentiometer potRight(PA_1, 3.3f, 100);

    // 3. Enable Bipolar Operation
    ENA = 1;   
    BIP1 = 1;  // Set to 1 to enable Bipolar PWM mode
    BIP2 = 1;  
    
    // In many bipolar configurations, DIR acts as a phase or enable for the H-bridge legs
    DIR1 = 1;  
    DIR2 = 1;

    lcd.cls();
    lcd.locate(0,0);
    lcd.printf("Bipolar Mode Active");

    while(true) {
        // 4. Read Potentiometer Values (0.0 to 1.0)
        float dutyL = potLeft.getCurrentSampleNorm();
        float dutyR = potRight.getCurrentSampleNorm();

        // 5. Apply to PWM Outputs
        // In Bipolar mode, the duty cycle controls the average voltage:
        // 0.5 (50%) is usually "Stop", 1.0 is "Full Forward", 0.0 is "Full Reverse"
        PWM1.write(dutyL);
        PWM2.write(dutyR);

        // 6. Update Telemetry
        lcd.locate(0,12);
        lcd.printf("L-PWM: %3.0f%%  R-PWM: %3.0f%%", dutyL*100, dutyR*100);
        
        heartbeat = !heartbeat;
        wait(0.1); 
    }
}

// FINAL
