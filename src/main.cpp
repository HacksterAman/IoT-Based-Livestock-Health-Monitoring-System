#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <DHT.h> // Include the DHT library
#include "MAX30100_PulseOximeter.h"

#define REPORTING_PERIOD_MS 1000

// Define OLED display width and height
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Initialize the display
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// PulseOximeter instance
PulseOximeter pox;

uint32_t previous = 0;
float h, t, f;

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

  // Display data on OLED
  display.clearDisplay();
  display.setCursor(0, 0); // Start at top-left corner
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
  display.display(); // Send buffer to display

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
