/**
 * @file globals.cpp
 * @brief Global system variables, hardware configuration, and shared state definitions.
 *
 * This module defines all global variables used throughout the CO2 detection system.
 * It serves as the central repository for:
 *
 *  - Hardware pin assignments and peripheral objects
 *  - Sensor calibration constants and current readings
 *  - Timing and sampling configuration
 *  - System state flags and status variables
 *  - Warning thresholds and control parameters
 *
 * Technical References:
 * [1] MQ-135 Gas Sensor Datasheet, Zhengzhou Winsen Electronics, v2.3, 2014
 *     Available: https://www.elprocus.com/mq135-air-quality-sensor/
 * [2] Arduino Uno R3 Schematic and Pin Mapping, Arduino LLC, 2023
 *     Available: https://docs.arduino.cc/hardware/uno-rev3
 * [3] 1602A Character LCD Datasheet, Hitachi HD44780 controller, 2005
 *     Available: https://www.waveshare.com/datasheet/LCD_en_PDF/LCD1602.pdf
 *     Reference: https://www.youtube.com/watch?v=g_6OJDyUw1w&t=184s 
 * [4] SG90 Micro Servo Datasheet, TowerPro, 2018
 *     Available: https://www.friendlywire.com/projects/ne555-servo-safe/SG90-datasheet.pdf 
 * [5] Nyquist-Shannon Sampling Theorem, 1949
 *     Available: https://en.wikipedia.org/wiki/Nyquist%E2%80%93Shannon_sampling_theorem
 *     Applied: 50Hz sampling for <25Hz signal components
 * [6] Carbon Dioxide Levels Chart
 *     Available: https://www.co2meter.com/blogs/news/carbon-dioxide-indoor-levels-chart 
 *
 * Design Philosophy:
 *  - All hardware-dependent values are defined here for easy modification
 *  - Sensor calibration values follow MQ-135 datasheet specifications [1]
 *  - Timing constants balance responsiveness with processing overhead
 *  - State flags ensure consistent behavior across modules
 *
 * Safety Considerations:
 *  - PPM_THRESHOLD set conservatively for early warning (2000 ppm)
 *  - Sensor voltage threshold provides hardware-level failsafe
 *  - Buzzer control variables prevent runaway audible alerts
 *  - Preheating flag ensures sensor stability before operation
 *
 * Maintenance Notes:
 *  - Modify pin assignments here when changing hardware connections
 *  - Adjust SAMPLES_PER_READING for different filtering characteristics
 *  - Update timing constants based on operational requirements
 *  - R0 initial value (76.63 kOhm) is a typical baseline; actual calibration required
 * 
 */

#include "globals.h"

//============================================================================
// HARDWARE PIN CONFIGURATION
//============================================================================
// Based on Arduino Uno R3 pin mapping and peripheral specifications [2]
// Pin assignments follow standard Arduino conventions for clarity

// Air Quality Sensor pins
int CO2_analog_pin = A0;  // MQ135 Analog  Output on A0 [1: Pin 4, AO]
int CO2_digital_pin = 4;  // MQ135 Digital Output on D4 [1: Pin 3, DO]
int LED_output = 13;      // Warning LED on D13 (built-in LED)
int Buzzer_output = 11;   // Piezo buzzer on D11 (PWM capable for tone control)

// Servo pin
Servo DoorServo;                         // SG90 servo object [4]
const int servoPin = 5;                  // Servo control signal on D5 

// LCD pin connections (1602A with HD44780 controller) [3]
const int rs = 2;  // Register Select on D2 [3: Pin 4]
const int en = 3;  // Enable pin on D3 [3: Pin 6]
const int d4 = 6;  // Data bit 4 on D6 [3: Pin 11]
const int d5 = 7;  // Data bit 5 on D7 [3: Pin 12]
const int d6 = 8;  // Data bit 6 on D8 [3: Pin 13]
const int d7 = 9;  // Data bit 7 on D9 [3: Pin 14]
                   // Data transfer is half byte per cycle

LiquidCrystal lcd(rs, en, d4, d5, d6, d7); // LCD object with 4-bit interface

//============================================================================
// SENSOR CALIBRATION
//============================================================================
// Constants derived from MQ-135 datasheet characteristics [1]
// Values assume clean air baseline of 400 ppm CO2 (standard outdoor concentration)

float R0 = 76.63;           // Baseline sensor resistance in clean air (kOhm) [1: Fig.3]
                            // Typical value from datasheet; calibrated at startup
                            // Used by calculatePPM(), updated by calibrateSensor();
const float RL = 20.0;      // Load resistance: 20 kOhm [1: Application circuit]
                            // Standard voltage divider value for MQ-135;
float originalR0 = 0;       // Reference R0 value from initial calibration
                            // Used for drift detection in quickRecalibrationCheck();
int adc = 0;                // Current ADC reading (0-1023)
                            // Updated by MQ135SensorDirectData(), used for diagnostics;
int d0 = 0;                 // Digital output state (HIGH/LOW)
                            // Factory-set threshold, used as hardware failsafe;
float sensor_voltage = 0.0; // Calculated sensor voltage (0-5V)
                            // Computed: adc × (5.0 / 1023.0), used by calculatePPM();

//============================================================================
// Timing & sampling
//============================================================================
// Sampling rates based on Nyquist-Shannon theorem [5] and MQ-135 response time

const int SAMPLES_PER_READING = 50;             // 50-sample moving average buffer
                                                // Provides 1-second window at 50Hz sampling
                                                // Balances noise rejection with responsiveness

float ppmReadings[SAMPLES_PER_READING] = {0};   // Circular buffer for PPM readings
                                                // Updated by updatePPMReading()
                                                // Averaged by getAveragePPM()

int readingIndex = 0;                           // Current position in circular buffer
                                                // Wraps using modulo arithmetic

unsigned long lastSampleTime = 0;               // Timestamp of last sensor sample
                                                // Ensures consistent 20ms (50Hz) sampling

const unsigned long WARNING_DISPLAY_TIME = 3000;  // 3-second prominent warning display
                                                  // Attention-grabbing period before detailed view

const unsigned long RECALIBRATION_INTERVAL = 300000;  // 5-minute (300,000 ms) recalibration
                                                      // Compensates for MQ-135 sensor drift [1: Stability]

//============================================================================
// Flags & states
//============================================================================
// State variables ensure consistent system behavior and prevent race conditions

bool isPreheated = false;           // Sensor warm-up completion flag
                                    // MQ-135 requires 20+ seconds for stable readings [1: Preheat]

bool isWarningActive = false;       // Current warning system state
                                    // Guards against duplicate activations

bool recalibrationDue = false;      // Scheduled recalibration pending flag
                                    // Set by checkRecalibration(), cleared by performRegularRecalibration()

bool skipPreheating = false;        // Debug/testing override (PRODUCTION: false)
                                    // Allows rapid development cycles

unsigned long lastCalibrationTime = 0;  // Timestamp of last calibration
                                        // Used with RECALIBRATION_INTERVAL for scheduling

unsigned long warningStartTime = 0;     // Timestamp when warning was activated
                                        // Used for WARNING_DISPLAY_TIME calculation

// Buzzer control variables (non-blocking pattern implementation)
unsigned long buzzerTimer = 0;      // Timestamp of last buzzer state change
                                    // Enables precise 500ms ON / 50ms OFF timing

bool buzzerState = false;           // Current buzzer output state (false=OFF, true=ON)
                                    // Toggled by updateBuzzer() based on timing

bool buzzerActive = false;          // Buzzer pattern activation flag
                                    // Set by startBuzzer(), cleared by stopBuzzer()

//============================================================================
// Thresholds
//============================================================================
// Safety limits based on indoor air quality standards and sensor characteristics

const int PPM_THRESHOLD = 2000;     // CO2 concentration warning threshold (ppm)
                                    // Based on [6]:
                                    //   - OSHA 8-hour exposure limit: 5000 ppm
                                    //   - ASHRAE comfort guideline: 1000 ppm
                                    //   - Conservative early warning: 2000 ppm 
                                    //     (We use this, can be changed to 1500 if user desires.)
                                    // Adjustable based on application requirements

const float SENSOR_VOLTAGE_THRESHOLD = 1.0;  // Raw voltage failsafe threshold (V)
                                             // Provides hardware-level protection
                                             // Corresponds to ~5000 ppm equivalent
                                             // Derived from empirical testing 
                                             // see test/CO2_testing.ipynb for data graphs. 1V
                                             // is an R0 independent threshold.
                                             //
                                             // i.e., all R0 values max out at 1V. If sensor exceeds 1V
                                             // something has gone very fucking wrong.