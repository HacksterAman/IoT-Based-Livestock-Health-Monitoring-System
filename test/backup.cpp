#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <DHT.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include "MAX30100_PulseOximeter.h"

#define REPORTING_PERIOD_MS 1000

// Wi-Fi credentials
const char *ssid = "Aman197";
const char *password = "ushaaman";

// Thingsboard server hostname and device token
const char *mqtt_server = "thingsboard.cloud";
const char *access_token = "UjLYcubWy9JTmYixsJnV";

// OLED display width and height
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Sensor setup
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
PulseOximeter pox;
Adafruit_MPU6050 mpu;
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

WiFiClient espClient;
PubSubClient client(espClient);

float h, t;
int heartRate = 0, spo2 = 0;
uint32_t previousMillis = 0;
uint32_t dhtMillis = 0;

void onBeatDetected()
{
    Serial.println("Beat detected!");
}

// Wi-Fi setup function
void setupWiFi()
{
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("WiFi connected");
}

// MQTT reconnect function
void reconnect()
{
    while (!client.connected())
    {
        Serial.print("Attempting MQTT connection...");
        if (client.connect("ESP32Client", access_token, NULL))
        {
            Serial.println("connected");
        }
        else
        {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}

void core0Task(void *parameter);
void core1Task(void *parameter);

void setup()
{
    Serial.begin(115200);
    setupWiFi();
    client.setServer(mqtt_server, 1883);

    Serial.println("Initializing sensors...");
    if (!pox.begin())
    {
        Serial.println("Failed to initialize PulseOximeter!");
        while (1)
            ;
    }
    pox.setOnBeatDetectedCallback(onBeatDetected);

    if (!mpu.begin())
    {
        Serial.println("Failed to initialize MPU6050!");
        while (1)
            ;
    }

    display.begin(0x3C);
    display.clearDisplay();
    display.setTextSize(1);

    dht.begin();

    // Create tasks pinned to different cores
    xTaskCreatePinnedToCore(core1Task, "Core1Task", 10000, NULL, 1, NULL, 1); // For reading MPU and MAX data
    xTaskCreatePinnedToCore(core0Task, "Core0Task", 10000, NULL, 1, NULL, 0); // For DHT11 and data upload
}

void loop()
{
}

// Task for Core 1: Read MPU6050 and Pulse Oximeter as fast as possible
void core1Task(void *parameter)
{
    for (;;)
    {
        // Read MAX30100 (Pulse Oximeter) data
        pox.update();
        heartRate = pox.getHeartRate();
        spo2 = pox.getSpO2();

        // Update OLED display
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
        display.display();

        // Print to Serial Monitor
        Serial.print("Heart rate: ");
        Serial.print(heartRate);
        Serial.print(" bpm, SpO2: ");
        Serial.print(spo2);
        Serial.print(" %, Temp: ");
        Serial.print(t);
        Serial.print(" C, Humidity: ");
        Serial.print(h);
        Serial.println(" %");

        // Read MPU6050 data
        sensors_event_t a, g, temp;
        mpu.getEvent(&a, &g, &temp);

        // Display MPU6050 data on Serial Monitor
        Serial.print("AccelX: ");
        Serial.print(a.acceleration.x);
        Serial.print(" AccelY: ");
        Serial.print(a.acceleration.y);
        Serial.print(" AccelZ: ");
        Serial.print(a.acceleration.z);
        Serial.print(" GyroX: ");
        Serial.print(g.gyro.x);
        Serial.print(" GyroY: ");
        Serial.print(g.gyro.y);
        Serial.print(" GyroZ: ");
        Serial.println(g.gyro.z);

        // Publish MPU6050 data to ThingsBoard immediately
        String mpuData = "{";
        mpuData += "\"Ax\":" + String(a.acceleration.x) + ",";
        mpuData += "\"Ay\":" + String(a.acceleration.y) + ",";
        mpuData += "\"Az\":" + String(a.acceleration.z) + ",";
        mpuData += "\"Gx\":" + String(g.gyro.x) + ",";
        mpuData += "\"Gy\":" + String(g.gyro.y) + ",";
        mpuData += "\"Gz\":" + String(g.gyro.z);
        mpuData += "}";
        client.publish("v1/devices/me/telemetry", mpuData.c_str());

        delay(10); // Small delay for stability
    }
}

// Task for Core 0: Read DHT11 every 2 seconds, upload all data every second
void core0Task(void *parameter)
{
    for (;;)
    {
        if (!client.connected())
        {
            reconnect();
        }
        client.loop();

        // Every 2 seconds, read DHT11 sensor data
        if (millis() - dhtMillis > 2000)
        {
            h = dht.readHumidity();
            t = dht.readTemperature();
            dhtMillis = millis();
            if (isnan(h) || isnan(t))
            {
                Serial.println("Failed to read from DHT sensor!");
            }
        }

        // Every second, publish data to ThingsBoard
        if (millis() - previousMillis > 1000)
        {
            // Create JSON payload with all sensor data
            String payload = "{";
            payload += "\"heartrate\":" + String(heartRate) + ",";
            payload += "\"spo2\":" + String(spo2) + ",";
            payload += "\"temperature\":" + String(t) + ",";
            payload += "\"humidity\":" + String(h);
            payload += "}";

            client.publish("v1/devices/me/telemetry", payload.c_str());
            previousMillis = millis();
        }
    }
}
