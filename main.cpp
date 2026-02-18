#include "mbed.h"
#include "C12832.h"
#include "mbed2/299/TARGET_NUCLEO_F401RE/TARGET_STM/TARGET_STM32F4/TARGET_NUCLEO_F401RE/PinNames.h"

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
PwmOut      PWM1(PB_15); 
DigitalOut  DIR1(PC_14);  
DigitalOut  BIP1(PA_13);  

// Motor 2 (Right)
PwmOut      PWM2(PB_14);  
DigitalOut  DIR2(PB_2); 
DigitalOut  BIP2(PA_14);  

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

// =============================================================
// GLOBALS & CONSTANTS
// =============================================================
static const float VLOGIC = 3.3f;
static const int   PWM_PERIOD_US = 50; 
static const float POT_FS = 50.0f;
static const float DEAD_BAND = 0.03f;

volatile long pulseL = 0;
volatile long pulseR = 0;
volatile bool dirToggleRequested = false;

void onButtonPress() { dirToggleRequested = true; }
void onPulseL() { (encLB.read() == 0) ? pulseL++ : pulseL--; }
void onPulseR() { (encRB.read() == 0) ? pulseR++ : pulseR--; }

static inline float clamp01(float x) {
    if (x < 0.0f) return 0.0f;
    if (x > 1.0f) return 1.0f;
    return x;
}

// =============================================================
// MAIN PROGRAM
// =============================================================
int main() {
    encLA.mode(PullUp); encLB.mode(PullUp);
    encRA.mode(PullUp); encRB.mode(PullUp);
    userBtn.mode(PullUp);

    SamplingPotentiometer potLeft(A0, VLOGIC, POT_FS);
    SamplingPotentiometer potRight(A1, VLOGIC, POT_FS);

    PWM1.period_us(PWM_PERIOD_US);
    PWM2.period_us(PWM_PERIOD_US);

    BIP1 = 0; BIP2 = 0; 
    ENA = 1; 
    DIR1 = 1; DIR2 = 0;

    userBtn.fall(&onButtonPress);
    encLA.rise(&onPulseL);
    encRA.rise(&onPulseR);

    wait_ms(100); 
    
    Timer lcdTimer;
    lcdTimer.start();

    while (true) {
        static int h = 0;
        if(++h > 20) { heartbeat = !heartbeat; h = 0; }

        if (dirToggleRequested) {
            DIR1 = !DIR1; DIR2 = !DIR2;
            dirToggleRequested = false;
            wait_ms(150); 
        }

        // --- Process Left Motor ---
        float normL = clamp01(potLeft.getCurrentSampleNorm());
        if (normL < DEAD_BAND) normL = 0.0f;
        if (normL > (1.0f - DEAD_BAND)) normL = 1.0f;
        PWM1.write(1.0f - normL); 

        // --- Process Right Motor ---
        float normR = clamp01(potRight.getCurrentSampleNorm());
        if (normR < DEAD_BAND) normR = 0.0f;
        if (normR > (1.0f - DEAD_BAND)) normR = 1.0f;
        PWM2.write(1.0f - normR); 

        // --- Update LCD (Both Pots and Both Encoders) ---
        if (lcdTimer.read_ms() >= 200) {
            lcd.cls();
            
            // Top Row: Encoder Counts
            lcd.locate(0, 0);
            lcd.printf("L-Enc:%-5ld  R-Enc:%-5ld", pulseL, pulseR);
            
            // Middle Row: Duty Cycles
            lcd.locate(0, 10);
            lcd.printf("L-Dty:%3.0f%%   R-Dty:%3.0f%%", normL*100, normR*100);

            // Bottom Row: Direction and Logic Volts
            lcd.locate(0, 20);
            lcd.printf("DIR:%s  L:%1.1fV  R:%1.1fV", 
                        DIR1 ? "FWD" : "REV", 
                        potLeft.getCurrentSampleVolts(), 
                        potRight.getCurrentSampleVolts());

            lcdTimer.reset();
        }
        wait_ms(10);
    }
}