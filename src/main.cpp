#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <DHT.h>
#include "MAX30100_PulseOximeter.h"
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <PubSubClient.h>
#include <WiFi.h>

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

// MQTT client setup
const char *mqtt_server = "thingsboard.cloud";
const char *access_token = "UjLYcubWy9JTmYixsJnV"; // Your access token
WiFiClient espClient;                              // Ensure you have included the WiFi library
PubSubClient client(espClient);

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

// Function to connect to the MQTT server
void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ArduinoClient", access_token, NULL))
    {
      Serial.println("connected");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      delay(2000);
    }
  }
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
  {
    Serial.println("MPU6050 Found!");
  }

  mpu.setHighPassFilter(MPU6050_HIGHPASS_0_63_HZ);
  mpu.setMotionDetectionThreshold(1);
  mpu.setMotionDetectionDuration(20);
  mpu.setInterruptPinLatch(true);
  mpu.setInterruptPinPolarity(true);
  mpu.setMotionInterrupt(true);

  Serial.print("Initializing pulse oximeter..");
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

  pox.setOnBeatDetectedCallback(onBeatDetected);
  display.begin(0x3C);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  dht.begin();

  // Connect to WiFi (Ensure you have your WiFi credentials)
  WiFi.begin("yourSSID", "yourPASSWORD");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");

  client.setServer(mqtt_server, 1883);
}

void loop()
{
  // Ensure MQTT connection
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();

  // Update pulse oximeter
  pox.update();
  int heartRate = pox.getHeartRate();
  int spo2 = pox.getSpO2();

  // Read DHT11 every 2 seconds
  if (millis() - previous > 2000)
  {
    h = dht.readHumidity();
    t = dht.readTemperature();
    if (isnan(t) || isnan(h))
    {
      Serial.println("Failed to read from DHT sensor!");
    }
    previous = millis();
  }

  // Check for motion interrupt and get MPU6050 data
  if (mpu.getMotionInterruptStatus())
  {
    mpu.getEvent(&a, &g, &temp);
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
  display.display();

  // Publish sensor data to ThingsBoard
  String sensorData = "temperature=" + String(t) + "&humidity=" + String(h) +
                      "&heartrate=" + String(heartRate) + "&spo2=" + String(spo2) +
                      "&Gx=" + String(g.gyro.x) + "&Gy=" + String(g.gyro.y) +
                      "&Gz=" + String(g.gyro.z) + "&Ax=" + String(a.acceleration.x) +
                      "&Ay=" + String(a.acceleration.y) + "&Az=" + String(a.acceleration.z);

  client.publish("v1/devices/me/telemetry", sensorData.c_str());

  // Print to Serial
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
