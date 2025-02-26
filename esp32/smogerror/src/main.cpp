#include <Wire.h>
#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>

// I2C Address of Arduino Uno (Slave)
#define SLAVE_ADDRESS 0x08

// Wi-Fi Credentials
const char* WIFI_SSID = "hi";
const char* WIFI_PASSWORD = "11111111";

// Firebase Configuration
#define FIREBASE_HOST "https://smog-tower-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define FIREBASE_AUTH "AIzaSyCKipnnsaq1vfaeCYmhuMI_mwaWSVVcP1M"

// Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Gas names and sensor values
String gases[] = {"Ammonia", "Benzene", "CO", "CNG", "LPG", "Hydrogen", "Smoke"};
float sensorValues[7];

// DHT22 Setup
#define DHTPIN 4   // Change this to the pin your DHT22 sensor is connected to
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

void setup() {
    // Start Serial and I2C
    Serial.begin(115200);
    Wire.begin();

    // Connect to Wi-Fi
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(1000);
    }
    Serial.println("\nWi-Fi Connected!");

    // Initialize Firebase
    config.host = FIREBASE_HOST;
    config.signer.tokens.legacy_token = FIREBASE_AUTH;

    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);

    if (Firebase.ready()) {
        Serial.println("Firebase connected successfully.");
    } else {
        Serial.println("Failed to connect to Firebase. Check credentials.");
    }

    // Initialize DHT sensor
    dht.begin();
}

void loop() {
    // Request data from the Arduino Uno slave
    Wire.requestFrom(SLAVE_ADDRESS, sizeof(float) * 7);

    // Read gas sensor values
    for (int i = 0; i < 7; i++) {
        if (Wire.available()) {
            Wire.readBytes((char*)&sensorValues[i], sizeof(sensorValues[i]));
        }
    }

    // Print gas sensor readings
    Serial.println("Gas Sensor Readings:");
    for (int i = 0; i < 7; i++) {
        Serial.print(gases[i]);
        Serial.print(": ");
        Serial.print(sensorValues[i], 2);
        Serial.println(" ppm");
    }

    // Upload gas data to Firebase
    FirebaseJson json;
    for (int i = 0; i < 7; i++) {
        json.set(gases[i].c_str(), sensorValues[i]);
    }

    if (Firebase.RTDB.setJSON(&fbdo, "/gas_readings", &json)) {
        Serial.println("Gas data uploaded successfully!");
    } else {
        Serial.printf("Firebase gas data upload failed: %s\n", fbdo.errorReason().c_str());
    }

    // Read temperature and humidity from DHT22
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();

    if (!isnan(temperature) && !isnan(humidity)) {
        Serial.printf("Temperature: %.2f°C, Humidity: %.2f%%\n", temperature, humidity);

        // Upload Temperature separately
        if (Firebase.RTDB.setFloat(&fbdo, "/sensorData/temperature", temperature)) {
            Serial.println("Temperature uploaded successfully!");
        } else {
            Serial.printf("Firebase temperature upload failed: %s\n", fbdo.errorReason().c_str());
        }

        // Upload Humidity separately
        if (Firebase.RTDB.setFloat(&fbdo, "/sensorData/humidity", humidity)) {
            Serial.println("Humidity uploaded successfully!");
        } else {
            Serial.printf("Firebase humidity upload failed: %s\n", fbdo.errorReason().c_str());
        }
    } else {
        Serial.println("Failed to read from DHT sensor!");
    }

    delay(5000);  // Wait before the next reading
}
