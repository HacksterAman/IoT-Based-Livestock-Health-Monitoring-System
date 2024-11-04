#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <DHT.h>
#include "MAX30100_PulseOximeter.h"
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#define LED_BUILTIN 2

// Wi-Fi credentials
const char *ssid = "ID";
const char *password = "PASSWORD";

// Thingsboard server hostname and device token
const char *mqtt_server = "thingsboard.cloud";
const char *access_token = "UjLYcubWy9JTmYixsJnV";

// Define OLED display width and height
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Initialize the display
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// PulseOximeter instance
PulseOximeter pox;

// MPU6050 instance
Adafruit_MPU6050 mpu;
sensors_event_t a, g, temp;

// Global Variables
uint32_t previous = 0;
float h, t;
int heartRate, spo2;
WiFiClient espClient;
PubSubClient client(espClient);

// DHT11 setup
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

void setupWiFi();
void reconnect();
void core0Task(void *parameter);
void core1Task(void *parameter);

void setup()
{
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);

  // Initialize SH1106G
  display.begin(0x3C);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);

  // Initialize MPU6050
  mpu.begin();
  mpu.setHighPassFilter(MPU6050_HIGHPASS_0_63_HZ);
  mpu.setMotionDetectionThreshold(1);
  mpu.setMotionDetectionDuration(20);
  mpu.setInterruptPinLatch(true);
  mpu.setInterruptPinPolarity(true);
  mpu.setMotionInterrupt(true);

  // Initialize DHT11
  dht.begin();

  // Initialize MAX30100
  pox.begin();

  // Create tasks pinned to different cores
  xTaskCreatePinnedToCore(core1Task, "Core0Task", 10000, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(core0Task, "Core1Task", 10000, NULL, 1, NULL, 1);
}

void loop();

void core0Task(void *parameter)
{
  setupWiFi();
  client.setServer(mqtt_server, 1883);
  for (;;)
  {
    if (!client.connected())
    {
      reconnect();
    }
    client.loop();

    String payload = "{";
    payload += "\"heartrate\":" + String(heartRate) + ",";
    payload += "\"spo2\":" + String(spo2) + ",";
    payload += "\"temperature\":" + String(t) + ",";
    payload += "\"humidity\":" + String(h) + ",";
    payload += "\"Ax\":" + String(a.acceleration.x) + ",";
    payload += "\"Ay\":" + String(a.acceleration.y) + ",";
    payload += "\"Az\":" + String(a.acceleration.z) + ",";
    payload += "\"Gx\":" + String(g.gyro.x) + ",";
    payload += "\"Gy\":" + String(g.gyro.y) + ",";
    payload += "\"Gz\":" + String(g.gyro.z);
    payload += "}";
    client.publish("v1/devices/me/telemetry", payload.c_str());
    delay(100);
  }
}

void core1Task(void *parameter)
{
  for (;;)
  {
    // Make sure to call update as fast as possible
    pox.update();
    heartRate = pox.getHeartRate();
    spo2 = pox.getSpO2();

    // Check for motion interrupt
    if (mpu.getMotionInterruptStatus())
    {

      mpu.getEvent(&a, &g, &temp);

      // Serial.print("AccelX: ");
      // Serial.print(a.acceleration.x);
      // Serial.print(", AccelY: ");
      // Serial.print(a.acceleration.y);
      // Serial.print(", AccelZ: ");
      // Serial.print(a.acceleration.z);
      // Serial.print(", GyroX: ");
      // Serial.print(g.gyro.x);
      // Serial.print(", GyroY: ");
      // Serial.print(g.gyro.y);
      // Serial.print(", GyroZ: ");
      // Serial.println(g.gyro.z);
    }

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

    // Serial.print("Heart rate: ");
    // Serial.print(heartRate);
    // Serial.print(" bpm / SpO2: ");
    // Serial.print(spo2);
    // Serial.print(" % / Temp: ");
    // Serial.print(t);
    // Serial.print(" C / Humidity: ");
    // Serial.print(h);
    // Serial.println(" %");
  }
}

// Wi-Fi setup function
void setupWiFi()
{
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
  }
  Serial.println("WiFi connected");
  digitalWrite(LED_BUILTIN, HIGH);
}

// MQTT reconnect function
void reconnect()
{
  while (!client.connected())
  {
    digitalWrite(LED_BUILTIN, LOW);
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Client", access_token, NULL))
    {
      Serial.println("connected");
      digitalWrite(LED_BUILTIN, HIGH);
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