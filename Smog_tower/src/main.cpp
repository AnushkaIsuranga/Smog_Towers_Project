//Libraries
#include <LiquidCrystal_I2C.h>  
#include "BluetoothSerial.h"
#include <Wire.h>
#include <WiFi.h>
#include <time.h>
#include <MQ135.h>
#include "DHT.h"
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>

//LED
#define LOW_LED 25 
#define MODARATE_LED 26 
#define HIGH_LED 27
#define CRITICAL_LED 33

// sensors
#define MQ2_PIN 35
#define MQ4_PIN 32
#define MQ7_PIN 36
#define MQ8_PIN 39
#define MQ135_PIN 12
#define C6H6_PIN 13
#define HUMIDITY_SENSOR_PIN 15

// Firebase and WiFi credentials
#define FIREBASE_HOST "https://smog-tower-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define FIREBASE_AUTH "AIzaSyCKipnnsaq1vfaeCYmhuMI_mwaWSVVcP1M"

//WiFi
#define WIFI_SSID "Redmi 13" 
#define WIFI_PASSWORD "123456789" 

// initial
DHT dht(HUMIDITY_SENSOR_PIN, DHT22);
LiquidCrystal_I2C lcd(0x27, 16, 2);
BluetoothSerial SerialBT;
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;

// Ro values for each sensor (clean air reference resistance)
const float RO_MQ2 = 1.0;  // kΩ
const float RO_MQ4 = 0.50;   // kΩ
const float RO_MQ7 = 1.0;  // kΩ
const float RO_MQ8 = 1.0;  // kΩ
const float RO_MQ135 = 3.0; // kΩ
const float RO_C6H6 = 5.0;  // kΩ (approximately, check datasheet for exact value)

// Function to calculate the concentration of gases in ppm
float getConcentration(int sensorValue, float Ro, float min, float max) {
    // Calculate sensor resistance (Ro)
    float sensorResistance = (float)(9095 - sensorValue) / sensorValue; // Adjust this based on your sensor setup
    // Calculate gas concentration based on Ro and sensor reading
    float concentration = (Ro / sensorResistance) * (max - min) + min;
    return concentration;
}

// Function to map raw sensor value to gas concentration and determine saturation levels
String getGasSaturation(int sensorValue, String gasName, float Ro, float min, float max, float lowThreshold, float moderateThreshold, float highThreshold) {
    // Get gas concentration in ppm
    float concentration = getConcentration(sensorValue, Ro, min, max);

    // Determine the saturation level based on thresholds
    String saturation;
    if (concentration < lowThreshold) {
        saturation = "Low";
    } else if (concentration < moderateThreshold) {
        saturation = "Moderate";
    } else if (concentration < highThreshold) {
        saturation = "High";
    } else {
        saturation = "Critical";
    }

    // Return the formatted result
    return gasName + ": " + String(concentration, 1) + "ppm    (" + saturation + ")";
}

// Function to extract saturation from the result
String getSaturationLevel(int sensorValue, String gasName, float Ro, float min, float max, float lowThreshold, float moderateThreshold, float highThreshold) {
    String gasSaturation = getGasSaturation(sensorValue, gasName, Ro, min, max, lowThreshold, moderateThreshold, highThreshold);
    
    // Extract saturation level from the formatted result
    int saturationStartIndex = gasSaturation.indexOf("(") + 1;
    int saturationEndIndex = gasSaturation.indexOf(")");
    
    return gasSaturation.substring(saturationStartIndex, saturationEndIndex); // Return just the saturation part
}

// Control LEDs based on saturation level
void controlLEDs(String saturation, int lowLedPin, int moderateLedPin, int highLedPin, int criticalLedPin) {
    // Turn off all LEDs initially
    digitalWrite(LOW_LED, LOW);
    digitalWrite(MODARATE_LED, LOW);
    digitalWrite(HIGH_LED, LOW);
    digitalWrite(CRITICAL_LED, LOW);

    // Turn on the appropriate LED based on the saturation level
    if (saturation == "Low") {
        digitalWrite(LOW_LED, HIGH);
    } else if (saturation == "Moderate") {
        digitalWrite(MODARATE_LED, HIGH);
    } else if (saturation == "High") {
        digitalWrite(HIGH_LED, HIGH);
    } else if (saturation == "Critical") {
        digitalWrite(CRITICAL_LED, HIGH);
    }
}


const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 3600;

void setup() {
  // Seria l print
  Serial.begin(115200); 
  SerialBT.begin("ESP32_GasSensor");

  //LED output
  pinMode(LOW_LED,  OUTPUT);
  pinMode(MODARATE_LED, OUTPUT);
  pinMode(HIGH_LED, OUTPUT);
  pinMode(CRITICAL_LED, OUTPUT);

  // sensorpins
  pinMode (MQ2_PIN,  INPUT);
  pinMode(MQ4_PIN, INPUT);
  pinMode(MQ7_PIN, INPUT);
  pinMode(MQ8_PIN, INPUT);
  pinMode(MQ135_PIN, INPUT);
  pinMode(C6H6_PIN, INPUT);
  dht.begin();
  Wire.begin(21, 22); 
  lcd.init();     
  lcd.setBacklight(LOW);
  Serial.println("System Initialized");

  // WiFi connection
  Serial.print("Connecting to WiFi");

  // Connecting to WiFi with a visual update on the LCD
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED) {
    lcd.setBacklight(HIGH);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Connecting to");
    lcd.setCursor(0, 1);
    lcd.print("WiFi");
    for (int i = 0; i < 3; i++) {
      lcd.setCursor(4 + i, 1);
      lcd.print(".");
      delay(500);
    }

    delay(1000);
  }

  Serial.println("\nConnected to WiFi!");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connected to");
  lcd.setCursor(0, 1);
  lcd.print("WiFi!");
  delay(2000);  // Display the "Connected to WiFi!" message for 2 seconds
  
  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  if (Firebase.ready()) {
      Serial.println("Firebase connected successfully.");
  } else {
      Serial.println("Failed to connect to Firebase. Check credentials.");
  }

  Serial.println("System Initialized");
}

void printSensorDataToSerial() {
  // Print to Bluetooth Serial
  // MQ2 Sensor: LPG, Methane, Butane, Hydrogen, Smoke, Alcohol, Propane
  SerialBT.println("MQ2 Sensor Detected:");
  SerialBT.println(getGasSaturation(analogRead(MQ2_PIN), "LPG", RO_MQ2, 200, 5000, 800, 2000, 4000));
  SerialBT.println(getGasSaturation(analogRead(MQ2_PIN), "Methane (CH₄)", RO_MQ2, 100, 3000, 500, 1500, 2500));
  SerialBT.println(getGasSaturation(analogRead(MQ2_PIN), "Butane (C₄H₁₀)", RO_MQ2, 100, 2500, 400, 1200, 2000));
  SerialBT.println(getGasSaturation(analogRead(MQ2_PIN), "Hydrogen (H₂)", RO_MQ2, 50, 2000, 200, 800, 1500));
  SerialBT.println(getGasSaturation(analogRead(MQ2_PIN), "Smoke", RO_MQ2, 100, 3000, 500, 1500, 2500));
  SerialBT.println(getGasSaturation(analogRead(MQ2_PIN), "Alcohol", RO_MQ2, 200, 4000, 1000, 2000, 3000));
  SerialBT.println(getGasSaturation(analogRead(MQ2_PIN), "Propane (C₃H₈)", RO_MQ2, 50, 1000, 200, 500, 800));

  // MQ4 Sensor: Methane, CNG
  SerialBT.println("MQ4 Sensor Detected:");
  SerialBT.println(getGasSaturation(analogRead(MQ4_PIN), "Methane (CH₄)", RO_MQ4, 100, 3000, 500, 1500, 2500));
  SerialBT.println(getGasSaturation(analogRead(MQ4_PIN), "CNG", RO_MQ4, 100, 4000, 800, 2000, 3500));

  // MQ7 Sensor: Carbon Monoxide
  SerialBT.println("MQ7 Sensor Detected:");
  SerialBT.println(getGasSaturation(analogRead(MQ7_PIN), "Carbon Monoxide (CO)", RO_MQ7, 50, 1000, 200, 500, 800));

  // MQ8 Sensor: Hydrogen
  SerialBT.println("MQ8 Sensor Detected:");
  SerialBT.println(getGasSaturation(analogRead(MQ8_PIN), "Hydrogen (H₂)", RO_MQ8, 50, 2000, 200, 800, 1500));

  // MQ135 Sensor: Ammonia, Benzene, Alcohol
  SerialBT.println("MQ135 Sensor Detected:");
  SerialBT.println(getGasSaturation(analogRead(MQ135_PIN), "Ammonia (NH₃)", RO_MQ135, 10, 500, 50, 200, 400));
  SerialBT.println(getGasSaturation(analogRead(MQ135_PIN), "Benzene (C₆H₆)", RO_MQ135, 1, 200, 50, 100, 150));
  SerialBT.println(getGasSaturation(analogRead(MQ135_PIN), "Alcohol", RO_MQ135, 10, 1000, 200, 500, 800));

  // C6H6 Sensor: Benzene
  SerialBT.println("C6H6 Sensor Detected:");
  SerialBT.println(getGasSaturation(analogRead(C6H6_PIN), "Benzene (C₆H₆)", RO_C6H6, 1, 200, 50, 100, 150));
  
  SerialBT.println("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
  
  // Print to Serial Monitor
  // MQ2 Sensor: LPG, Methane, Butane, Hydrogen, Smoke, Alcohol, Propane
  Serial.println("MQ2 Sensor Detected:");
  Serial.println(getGasSaturation(analogRead(MQ2_PIN), "LPG", RO_MQ2, 200, 5000, 800, 2000, 4000));
  Serial.println(getGasSaturation(analogRead(MQ2_PIN), "Methane (CH₄)", RO_MQ2, 100, 3000, 500, 1500, 2500));
  Serial.println(getGasSaturation(analogRead(MQ2_PIN), "Butane (C₄H₁₀)", RO_MQ2, 100, 2500, 400, 1200, 2000));
  Serial.println(getGasSaturation(analogRead(MQ2_PIN), "Hydrogen (H₂)", RO_MQ2, 50, 2000, 200, 800, 1500));
  Serial.println(getGasSaturation(analogRead(MQ2_PIN), "Smoke", RO_MQ2, 100, 3000, 500, 1500, 2500));
  Serial.println(getGasSaturation(analogRead(MQ2_PIN), "Alcohol", RO_MQ2, 200, 4000, 1000, 2000, 3000));
  Serial.println(getGasSaturation(analogRead(MQ2_PIN), "Propane (C₃H₈)", RO_MQ2, 50, 1000, 200, 500, 800));

  // MQ4 Sensor: Methane, CNG
  Serial.println("MQ4 Sensor Detected:");
  Serial.println(getGasSaturation(analogRead(MQ4_PIN), "Methane (CH₄)", RO_MQ4, 100, 3000, 500, 1500, 2500));
  Serial.println(getGasSaturation(analogRead(MQ4_PIN), "CNG", RO_MQ4, 100, 4000, 800, 2000, 3500));

  // MQ7 Sensor: Carbon Monoxide
  Serial.println("MQ7 Sensor Detected:");
  Serial.println(getGasSaturation(analogRead(MQ7_PIN), "Carbon Monoxide (CO)", RO_MQ7, 50, 1000, 200, 500, 800));

  // MQ8 Sensor: Hydrogen
  Serial.println("MQ8 Sensor Detected:");
  Serial.println(getGasSaturation(analogRead(MQ8_PIN), "Hydrogen (H₂)", RO_MQ8, 50, 2000, 200, 800, 1500));

  // MQ135 Sensor: Ammonia, Benzene, Alcohol
  Serial.println("MQ135 Sensor Detected:");
  Serial.println(getGasSaturation(analogRead(MQ135_PIN), "Ammonia (NH₃)", RO_MQ135, 10, 500, 50, 200, 400));
  Serial.println(getGasSaturation(analogRead(MQ135_PIN), "Benzene (C₆H₆)", RO_MQ135, 1, 200, 50, 100, 150));
  Serial.println(getGasSaturation(analogRead(MQ135_PIN), "Alcohol", RO_MQ135, 10, 1000, 200, 500, 800));

  // C6H6 Sensor: Benzene
  Serial.println("C6H6 Sensor Detected:");
  Serial.println(getGasSaturation(analogRead(C6H6_PIN), "Benzene (C₆H₆)", RO_C6H6, 1, 200, 50, 100, 150));
  
  Serial.println("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
}

// Define a non-linear correction function
float calibrateHumidity(float rawHumidity) {
  if (rawHumidity < 50.0) {
    return rawHumidity - 5.0;
  } else if (rawHumidity < 80.0) {
    return rawHumidity - 10.0;
  } else {
    return rawHumidity - 20.0;
  }
}

void updateFirebase() {
  // Reading Temperature & Humidity values
  float temperature = dht.readTemperature();
  float rawHumidity = dht.readHumidity();

  float calibratedHumidity = calibrateHumidity(rawHumidity);

  if (Firebase.ready() && (millis() - sendDataPrevMillis > 5000 || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();

    // DHT Sensor
    if (Firebase.RTDB.setFloat(&fbdo, "/DHT/Temperature", temperature)) {
      Serial.println("Temperature Data set successfully!");
    } else {
      Serial.println("Error setting temperature data: " + fbdo.errorReason());
    }
    if (Firebase.RTDB.setFloat(&fbdo, "/DHT/Humidity", calibratedHumidity)) {
        Serial.println("Humidity Data set successfully!");
    } else {
        Serial.println("Error setting humidity data: " + fbdo.errorReason());
    }

    // MQ2 Sensor: LPG, Methane, Butane, Hydrogen, Smoke, Alcohol, Propane
    Serial.println("MQ2 Sensor Detected:");
    if (Firebase.RTDB.set(&fbdo, "/MQ2/LPG", getGasSaturation(analogRead(MQ2_PIN), "LPG", RO_MQ2, 200, 5000, 800, 2000, 4000))) {
      Serial.println("LPG Data set successfully!");
    } else {
      Serial.println("Error setting LPG data: " + fbdo.errorReason());
    }
    if (Firebase.RTDB.set(&fbdo, "/MQ2/Methane", getGasSaturation(analogRead(MQ2_PIN), "Methane (CH₄)", RO_MQ2, 100, 3000, 500, 1500, 2500))) {
      Serial.println("Methane Data set successfully!");
    } else {
      Serial.println("Error setting Methane data: " + fbdo.errorReason());
    }
    if (Firebase.RTDB.set(&fbdo, "/MQ2/Butane", getGasSaturation(analogRead(MQ2_PIN), "Butane (C₄H₁₀)", RO_MQ2, 100, 2500, 400, 1200, 2000))) {
      Serial.println("Butane Data set successfully!");
    } else {
      Serial.println("Error setting Butane data: " + fbdo.errorReason());
    }
    if (Firebase.RTDB.set(&fbdo, "/MQ2/Hydrogen", getGasSaturation(analogRead(MQ2_PIN), "Hydrogen (H₂)", RO_MQ2, 50, 2000, 200, 800, 1500))) {
      Serial.println("Hydrogen Data set successfully!");
    } else {
      Serial.println("Error setting Hydrogen data: " + fbdo.errorReason());
    }
    if (Firebase.RTDB.set(&fbdo, "/MQ2/Smoke", getGasSaturation(analogRead(MQ2_PIN), "Smoke", RO_MQ2, 100, 3000, 500, 1500, 2500))) {
      Serial.println("Smoke Data set successfully!");
    } else {
      Serial.println("Error setting Smoke data: " + fbdo.errorReason());
    }
    if (Firebase.RTDB.set(&fbdo, "/MQ2/Alcohol", getGasSaturation(analogRead(MQ2_PIN), "Alcohol", RO_MQ2, 200, 4000, 1000, 2000, 3000))) {
      Serial.println("Alcohol Data set successfully!");
    } else {
      Serial.println("Error setting Alcohol data: " + fbdo.errorReason());
    }
    if (Firebase.RTDB.set(&fbdo, "/MQ2/Propane", getGasSaturation(analogRead(MQ2_PIN), "Propane (C₃H₈)", RO_MQ2, 50, 1000, 200, 500, 800))) {
      Serial.println("Propane Data set successfully!");
    } else {
      Serial.println("Error setting Propane data: " + fbdo.errorReason());
    }

    // MQ4 Sensor: Methane, CNG
    Serial.println("MQ4 Sensor Detected:");
    if (Firebase.RTDB.set(&fbdo, "/MQ4/Methane", getGasSaturation(analogRead(MQ4_PIN), "Methane (CH₄)", RO_MQ4, 100, 3000, 500, 1500, 2500))) {
      Serial.println("Methane (MQ4) Data set successfully!");
    } else {
      Serial.println("Error setting Methane (MQ4) data: " + fbdo.errorReason());
    }
    if (Firebase.RTDB.set(&fbdo, "/MQ4/CNG", getGasSaturation(analogRead(MQ4_PIN), "CNG", RO_MQ4, 100, 4000, 800, 2000, 3500))) {
      Serial.println("CNG Data set successfully!");
    } else {
      Serial.println("Error setting CNG data: " + fbdo.errorReason());
    }

    // MQ7 Sensor: Carbon Monoxide
    Serial.println("MQ7 Sensor Detected:");
    if (Firebase.RTDB.set(&fbdo, "/MQ7/CarbonMonoxide", getGasSaturation(analogRead(MQ7_PIN), "CarbonMonoxide (CO)", RO_MQ7, 50, 1000, 200, 500, 800))) {
      Serial.println("CarbonMonoxide Data set successfully!");
    } else {
      Serial.println("Error setting CarbonMonoxide data: " + fbdo.errorReason());
    }

    // MQ8 Sensor: Hydrogen
    Serial.println("MQ8 Sensor Detected:");
    if (Firebase.RTDB.set(&fbdo, "/MQ8/Hydrogen", getGasSaturation(analogRead(MQ8_PIN), "Hydrogen (H₂)", RO_MQ8, 50, 2000, 200, 800, 1500))) {
      Serial.println("Hydrogen (MQ8) Data set successfully!");
    } else {
      Serial.println("Error setting Hydrogen (MQ8) data: " + fbdo.errorReason());
    }

    // MQ135 Sensor: Ammonia, Benzene, Alcohol
    Serial.println("MQ135 Sensor Detected:");
    if (Firebase.RTDB.set(&fbdo, "/MQ135/Ammonia", getGasSaturation(analogRead(MQ135_PIN), "Ammonia (NH₃)", RO_MQ135, 10, 500, 50, 200, 400))) {
      Serial.println("Ammonia Data set successfully!");
    } else {
      Serial.println("Error setting Ammonia data: " + fbdo.errorReason());
    }
    if (Firebase.RTDB.set(&fbdo, "/MQ135/Benzene", getGasSaturation(analogRead(MQ135_PIN), "Benzene (C₆H₆)", RO_MQ135, 1, 200, 50, 100, 150))) {
      Serial.println("Benzene (C6H6) Data set successfully!");
    } else {
      Serial.println("Error setting Benzene (C6H6) data: " + fbdo.errorReason());
    }
    if (Firebase.RTDB.set(&fbdo, "/MQ135/Alcohol", getGasSaturation(analogRead(MQ135_PIN), "Alcohol", RO_MQ135, 10, 1000, 200, 500, 800))) {
      Serial.println("Alcohol (MQ135) Data set successfully!");
    } else {
      Serial.println("Error setting Alcohol (MQ135) data: " + fbdo.errorReason());
    }

    // C6H6 Sensor: Benzene
    Serial.println("C6H6 Sensor Detected:");
    if (Firebase.RTDB.set(&fbdo, "/C6H6/Benzene", getGasSaturation(analogRead(C6H6_PIN), "Benzene (C₆H₆)", RO_C6H6, 1, 200, 50, 100, 150))) {
      Serial.println("Benzene (C6H6) Data set successfully!");
    } else {
      Serial.println("Error setting Benzene (C6H6) data: " + fbdo.errorReason());
    }
  }
}

void loop() 
{
  // Readsensor
  int mq2_value = analogRead(MQ2_PIN);
  int mq4_value = analogRead(MQ4_PIN);
  int mq7_value = analogRead(MQ7_PIN);
  int mq8_value = analogRead(MQ8_PIN);
  int mq135_value = analogRead(MQ135_PIN);
  int c6h6_value = analogRead(C6H6_PIN);
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();  
  if (isnan(humidity) || isnan(temperature)) {
  Serial.println("Failed to read from DHT sensor!");
  } 
  else 
  {
    if (humidity < 50.0) {
      humidity -= 5.0;
    } else if (humidity < 80.0) {
      humidity -= 10.0;
    } else {
      humidity -= 25.0;
    }
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println(" °C");
    SerialBT.print("Temperature: ");
    SerialBT.print(temperature);
    SerialBT.println(" °C");
    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");
    SerialBT.print("Humidity: ");
    SerialBT.print(humidity);
    SerialBT.println(" %");
    // Display
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Temp: " + String(temperature) + "'C");
    lcd.setCursor(0, 1);
    lcd.print("Humidity: " + String(humidity) + "%");
    delay(3000);
  }

  // Define variables for each gas saturation
  String LPG = getGasSaturation(mq2_value, "LPG  ", RO_MQ2, 200, 5000, 800, 2000, 4000);
  String Methane_MQ2 = getGasSaturation(mq2_value, "Metha", RO_MQ2, 100, 3000, 500, 1500, 2500);
  String Butane_MQ2 = getGasSaturation(mq2_value, "Butan", RO_MQ2, 100, 2500, 400, 1200, 2000);
  String Smoke_MQ2 = getGasSaturation(mq2_value, "Smoke", RO_MQ2, 100, 3000, 500, 1500, 2500);
  String Alcohol_MQ2 = getGasSaturation(mq2_value, "Alcoh", RO_MQ2, 200, 4000, 1000, 2000, 3000);
  String Propane_MQ2 = getGasSaturation(mq2_value, "Propa", RO_MQ2, 50, 1000, 200, 500, 800);

  String CNG_MQ4 = getGasSaturation(mq4_value, "CNG  ", RO_MQ4, 100, 4000, 800, 2000, 3500);

  String CO_MQ7 = getGasSaturation(mq7_value, "CO   ", RO_MQ7, 50, 1000, 200, 500, 800);

  String Hydrogen_MQ8 = getGasSaturation(mq8_value, "H2   ", RO_MQ8, 50, 2000, 200, 800, 1500);

  String Ammonia_MQ135 = getGasSaturation(mq135_value, "Ammon", RO_MQ135, 10, 500, 50, 200, 400);
  String Benzene_MQ135 = getGasSaturation(mq135_value, "Benze", RO_MQ135, 1, 200, 50, 100, 150);

  // Get saturation level for each gas and store in a variable
  String LPG_Saturation = getSaturationLevel(mq2_value, "LPG  ", RO_MQ2, 200, 5000, 800, 2000, 4000);
  String Methane_Saturation = getSaturationLevel(mq2_value, "Metha", RO_MQ2, 100, 3000, 500, 1500, 2500);
  String Butane_Saturation = getSaturationLevel(mq2_value, "Butan", RO_MQ2, 100, 2500, 400, 1200, 2000);
  String Smoke_Saturation = getSaturationLevel(mq2_value, "Smoke", RO_MQ2, 100, 3000, 500, 1500, 2500);
  String Alcohol_Saturation = getSaturationLevel(mq2_value, "Alcoh", RO_MQ2, 200, 4000, 1000, 2000, 3000);
  String Propane_Saturation = getSaturationLevel(mq2_value, "Propa", RO_MQ2, 50, 1000, 200, 500, 800);

  String CNG_Saturation = getSaturationLevel(mq4_value, "CNG  ", RO_MQ4, 100, 4000, 800, 2000, 3500);

  String CO_Saturation = getSaturationLevel(mq7_value, "CO   ", RO_MQ7, 50, 1000, 200, 500, 800);

  String Hydrogen_Saturation = getSaturationLevel(mq8_value, "H2   ", RO_MQ8, 50, 2000, 200, 800, 1500);

  String Ammonia_Saturation = getSaturationLevel(mq135_value, "Ammon", RO_MQ135, 10, 500, 50, 200, 400);
  String Benzene_Saturation = getSaturationLevel(mq135_value, "Benze", RO_MQ135, 1, 200, 50, 100, 150);


  printSensorDataToSerial();
  // Control LED based on the sensors
  controlLEDs(LPG_Saturation, LOW_LED, MODARATE_LED, HIGH_LED, CRITICAL_LED);
  controlLEDs(Methane_Saturation, LOW_LED, MODARATE_LED, HIGH_LED, CRITICAL_LED);
  controlLEDs(Butane_Saturation, LOW_LED, MODARATE_LED, HIGH_LED, CRITICAL_LED);
  controlLEDs(Smoke_Saturation, LOW_LED, MODARATE_LED, HIGH_LED, CRITICAL_LED);
  controlLEDs(Alcohol_Saturation, LOW_LED, MODARATE_LED, HIGH_LED, CRITICAL_LED);
  controlLEDs(Propane_Saturation, LOW_LED, MODARATE_LED, HIGH_LED, CRITICAL_LED);
  controlLEDs(CNG_Saturation, LOW_LED, MODARATE_LED, HIGH_LED, CRITICAL_LED);
  controlLEDs(CO_Saturation, LOW_LED, MODARATE_LED, HIGH_LED, CRITICAL_LED);
  controlLEDs(Hydrogen_Saturation, LOW_LED, MODARATE_LED, HIGH_LED, CRITICAL_LED);
  controlLEDs(Ammonia_Saturation, LOW_LED, MODARATE_LED, HIGH_LED, CRITICAL_LED);
  controlLEDs(Benzene_Saturation, LOW_LED, MODARATE_LED, HIGH_LED, CRITICAL_LED);
    
  // LCD Print
  lcd.setBacklight(HIGH);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(LPG);  // Display LPG gas saturation
  lcd.setCursor(0, 1);
  lcd.print(Methane_MQ2);  // Display Methane (CH₄) from MQ2 sensor
  delay(3000);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(Butane_MQ2);  // Display Butane (C₄H₁₀) from MQ2 sensor
  lcd.setCursor(0, 1);
  lcd.print(Hydrogen_MQ8);  // Display Hydrogen (H₂) from MQ2 sensor
  delay(3000);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(Smoke_MQ2);  // Display Smoke from MQ2 sensor
  lcd.setCursor(0, 1);
  lcd.print(Alcohol_MQ2);  // Display Alcohol from MQ2 sensor
  delay(3000);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(Propane_MQ2);  // Display Propane (C₃H₈) from MQ2 sensor
  lcd.setCursor(0, 1);
  lcd.print(Ammonia_MQ135);  // Display Ammonia from MQ135 sensor
  delay(3000);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(CNG_MQ4);  // Display CNG from MQ4 sensor
  lcd.setCursor(0, 1);
  lcd.print(CO_MQ7);  // Display CO from MQ7 sensor
  delay(3000);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(Benzene_MQ135);  // Display Benzene (C₆H₆) from MQ135 sensor
  delay(3000);
  
  updateFirebase();
}