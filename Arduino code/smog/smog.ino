#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// I2C Address of Arduino Uno (Slave)
#define SLAVE_ADDRESS 0x08  

// Define sensor analog pins
#define MQ2_PIN   A0
#define MQ4_PIN   A1
#define MQ7_PIN   A2
#define MQ135_PIN A3

// Define LED pins
#define LOW_LED       6
#define MODERATE_LED  7
#define HIGH_LED      8
#define CRITICAL_LED  9

//fan eka
#define fan 12

// Constants for sensor calculations
#define RL_MQ 10.0  // Load resistance in kΩ
#define VCC 5.0     // Supply voltage

// Calibrated Ro values (set these after calibration in clean air)
float Ro_MQ2 = 10.0;
float Ro_MQ4 = 10.0;
float Ro_MQ7 = 10.0;
float Ro_MQ135 = 10.0;

// LCD configuration (I2C Address: 0x27 for most 16x2 LCDs)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Function to calculate sensor resistance
float getSensorResistance(int sensorValue) {
    float voltage = (sensorValue * VCC) / 1023.0;
    return ((VCC - voltage) * RL_MQ) / voltage;
}

// Function to get gas concentration using calibration curve
float getPPM(float sensorRs, float Ro, float A, float B) {
    return A * pow((sensorRs / Ro), B);  // A & B are gas-specific constants
}

// Function to read and convert sensor values to PPM
float readSensor(int pin, float Ro, float A, float B) {
    int sensorValue = analogRead(pin);
    float sensorRs = getSensorResistance(sensorValue);
    return getPPM(sensorRs, Ro, A, B);
}

// Function to determine saturation level
String getSaturationLevel(float ppm, float lowThreshold, float moderateThreshold, float highThreshold) {
    if (ppm < lowThreshold) return "Low";
    else if (ppm < moderateThreshold) return "Moderate";
    else if (ppm < highThreshold) return "High";
    else return "Critical";
}

// Function to control LEDs based on gas saturation
void controlLEDs(String saturation) {
    digitalWrite(LOW_LED, LOW);
    digitalWrite(MODERATE_LED, LOW);
    digitalWrite(HIGH_LED, LOW);
    digitalWrite(CRITICAL_LED, LOW);
    digitalWrite(fan ,LOW);

    if (saturation == "Low") digitalWrite(LOW_LED, HIGH);
    else if (saturation == "Moderate") digitalWrite(MODERATE_LED, HIGH);
    else if (saturation == "High") digitalWrite(HIGH_LED, HIGH), digitalWrite(fan,HIGH);
    else digitalWrite(CRITICAL_LED, HIGH);
}

// Function to send sensor data to ESP32 via I2C
void requestEvent() {
    // Read and calculate sensor data with high precision
    float ammonia = readSensor(MQ135_PIN, Ro_MQ135, 20.0, -0.3);
    float benzene = readSensor(MQ135_PIN, Ro_MQ135, 15.0, -0.4);
    float carbonMonoxide = readSensor(MQ7_PIN, Ro_MQ7, 25.0, -0.3);
    float cng = readSensor(MQ4_PIN, Ro_MQ4, 30.0, -0.5);
    float lpg = readSensor(MQ2_PIN, Ro_MQ2, 22.0, -0.4);
    float hydrogen = readSensor(MQ2_PIN, Ro_MQ2, 12.0, -0.5);
    float smoke = readSensor(MQ2_PIN, Ro_MQ2, 14.0, -0.4);

    // Send the individual readings to the master (ESP32)
    Wire.write((byte*)&ammonia, sizeof(ammonia));
    Wire.write((byte*)&benzene, sizeof(benzene));
    Wire.write((byte*)&carbonMonoxide, sizeof(carbonMonoxide));
    Wire.write((byte*)&cng, sizeof(cng));
    Wire.write((byte*)&lpg, sizeof(lpg));
    Wire.write((byte*)&hydrogen, sizeof(hydrogen));
    Wire.write((byte*)&smoke, sizeof(smoke));
}



// Function to display gas concentrations on LCD
void displayOnLCD() {
    String gases[] = {"Ammonia", "Benzene", "Alcohol", "CO", "Methane", "CNG", "LPG", "Butane", "Hydrogen", "Smoke", "Propane"};
    float values[] = {
        readSensor(MQ135_PIN, Ro_MQ135, 20.0, -0.3),
        readSensor(MQ135_PIN, Ro_MQ135, 15.0, -0.4),
        (readSensor(MQ135_PIN, Ro_MQ135, 10.0, -0.5) + readSensor(MQ2_PIN, Ro_MQ2, 8.0, -0.5)) / 2,
        readSensor(MQ7_PIN, Ro_MQ7, 25.0, -0.3),
        (readSensor(MQ4_PIN, Ro_MQ4, 18.0, -0.4) + readSensor(MQ2_PIN, Ro_MQ2, 10.0, -0.4)) / 2,
        readSensor(MQ4_PIN, Ro_MQ4, 30.0, -0.5),
        readSensor(MQ2_PIN, Ro_MQ2, 22.0, -0.4),
        readSensor(MQ2_PIN, Ro_MQ2, 15.0, -0.5),
        readSensor(MQ2_PIN, Ro_MQ2, 12.0, -0.5),
        readSensor(MQ2_PIN, Ro_MQ2, 14.0, -0.4),
        readSensor(MQ2_PIN, Ro_MQ2, 20.0, -0.4)
    };

    for (int i = 0; i < 11; i++) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(gases[i] + ": " + String(values[i], 1) + " ppm");
        delay(700);
    }
}

void setup() {
    Wire.begin(SLAVE_ADDRESS);
    Wire.onRequest(requestEvent);

    lcd.begin(16, 2);
    lcd.backlight();
    lcd.clear();

    pinMode(MQ2_PIN, INPUT);
    pinMode(MQ4_PIN, INPUT);
    pinMode(MQ7_PIN, INPUT);
    pinMode(MQ135_PIN, INPUT);

    pinMode(LOW_LED, OUTPUT);
    pinMode(MODERATE_LED, OUTPUT);
    pinMode(HIGH_LED, OUTPUT);
    pinMode(CRITICAL_LED, OUTPUT);

    pinMode(fan ,OUTPUT);

    Serial.begin(115200);
}

void loop() {
    displayOnLCD();

    float maxPPM = 0;
    maxPPM = max(maxPPM, readSensor(MQ2_PIN, Ro_MQ2, 20.0, -0.4));
    maxPPM = max(maxPPM, readSensor(MQ4_PIN, Ro_MQ4, 18.0, -0.4));
    maxPPM = max(maxPPM, readSensor(MQ7_PIN, Ro_MQ7, 25.0, -0.3));
    maxPPM = max(maxPPM, readSensor(MQ135_PIN, Ro_MQ135, 15.0, -0.4));

    String highestSaturation = getSaturationLevel(maxPPM, 10, 20, 30);
    controlLEDs(highestSaturation);


    delay(500);
} 