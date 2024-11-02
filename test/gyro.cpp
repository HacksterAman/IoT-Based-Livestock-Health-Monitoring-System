#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <DHT.h> // Include the DHT library
#include "MAX30100_PulseOximeter.h"
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#define REPORTING_PERIOD_MS 1000

// Define OLED display width and height
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Initialize the display
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// PulseOximeter instance
PulseOximeter pox;

// MPU6050 instance
Adafruit_MPU6050 mpu;

uint32_t previous = 0;
float h, t;

sensors_event_t a, g, temp;

// DHT11 setup
#define DHTPIN 4      // Pin where the DHT11 is connected
#define DHTTYPE DHT11 // DHT 11
DHT dht(DHTPIN, DHTTYPE);

// Callback fired when a pulse is detected
void onBeatDetected()
{
    Serial.println("Beat!");
}

void setup()
{
    Serial.begin(115200);

    // Initialize MPU6050
    Serial.println("Initializing MPU6050...");
    if (!mpu.begin())
    {
        Serial.println("Failed to find MPU6050 chip");
        for (;;)
            ;
    }
    else
        Serial.println("MPU6050 Found!");

    Serial.println("MPU6050 Found!");
    mpu.setHighPassFilter(MPU6050_HIGHPASS_0_63_HZ);
    mpu.setMotionDetectionThreshold(1);
    mpu.setMotionDetectionDuration(20);
    mpu.setInterruptPinLatch(true);
    mpu.setInterruptPinPolarity(true);
    mpu.setMotionInterrupt(true);

    Serial.print("Initializing pulse oximeter..");
    // Initialize the PulseOximeter instance
    if (!pox.begin())
    {
        Serial.println("FAILED");
        for (;;)
            ;
    }
    else
    {
        Serial.println("SUCCESS");
    }

    // Register a callback for the beat detection
    pox.setOnBeatDetectedCallback(onBeatDetected);

    // Initialize the OLED display
    display.begin(0x3C);

    display.clearDisplay();
    display.setTextSize(1);             // Normal 1:1 pixel scale
    display.setTextColor(SH110X_WHITE); // White text

    // Initialize DHT11 sensor
    dht.begin();
}

void loop()
{
    // Make sure to call update as fast as possible
    pox.update();
    int heartRate = pox.getHeartRate();
    int spo2 = pox.getSpO2();

    if (millis() - previous > 2000)
    {
        // Reading t or h takes about 250 milliseconds!
        // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
        h = dht.readHumidity();
        // Read temperature as Celsius (the default)
        t = dht.readTemperature();

        // Check if any reads failed and exit early (to try again).
        if (isnan(t) || isnan(h))
        {
            Serial.println("Failed to read from DHT sensor!");
        }
        previous = millis();
    }

    // Check for motion interrupt
    if (mpu.getMotionInterruptStatus())
    {

        mpu.getEvent(&a, &g, &temp);

        Serial.print("AccelX: ");
        Serial.print(a.acceleration.x);
        Serial.print(", AccelY: ");
        Serial.print(a.acceleration.y);
        Serial.print(", AccelZ: ");
        Serial.print(a.acceleration.z);
        Serial.print(", GyroX: ");
        Serial.print(g.gyro.x);
        Serial.print(", GyroY: ");
        Serial.print(g.gyro.y);
        Serial.print(", GyroZ: ");
        Serial.println(g.gyro.z);
    }

    // Display data on OLED
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Heart rate: ");
    display.print(heartRate);
    display.println(" bpm");
    display.print("SpO2: ");
    display.print(spo2);
    display.println(" %");
    display.print("Temp: ");
    display.print(t);
    display.println(" C");
    display.print("Humidity: ");
    display.print(h);
    display.println(" %");
    display.print("Ax:");
    display.print(a.acceleration.x);
    display.print(" | Gx:");
    display.println(g.gyro.x);
    display.print("Ay:");
    display.print(a.acceleration.y);
    display.print(" | Gy:");
    display.println(g.gyro.y);
    display.print("Az:");
    display.print(a.acceleration.z);
    display.print(" | Gz:");
    display.println(g.gyro.z);
    display.display();

    Serial.print("Heart rate: ");
    Serial.print(heartRate);
    Serial.print(" bpm / SpO2: ");
    Serial.print(spo2);
    Serial.print(" % / Temp: ");
    Serial.print(t);
    Serial.print(" C / Humidity: ");
    Serial.print(h);
    Serial.println(" %");
}
