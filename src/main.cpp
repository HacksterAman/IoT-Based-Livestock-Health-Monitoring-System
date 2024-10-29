#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include "MAX30100_PulseOximeter.h"

#define REPORTING_PERIOD_MS 1000

// Define OLED display width and height
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Initialize the display
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// PulseOximeter is the higher-level interface to the sensor
PulseOximeter pox;

// Callback fired when a pulse is detected
void onBeatDetected()
{
  Serial.println("Beat!");
  int heartRate = pox.getHeartRate();
  int spo2 = pox.getSpO2();

  // Display data on OLED
  display.clearDisplay();
  display.setCursor(0, 0); // Start at top-left corner
  display.print("Heart rate: ");
  display.print(heartRate);
  display.println(" bpm");
  display.print("SpO2: ");
  display.print(spo2);
  display.println(" %");
  display.display(); // Send buffer to display

  Serial.print("Heart rate: ");
  Serial.print(heartRate);
  Serial.print(" bpm / SpO2: ");
  Serial.print(spo2);
  Serial.println(" %");
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
  if (!display.begin(0x3C))
  {
    Serial.println(F("SH110X OLED not found"));
    for (;;)
      ;
  }
  display.clearDisplay();
  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setTextColor(SH110X_WHITE); // White text
}

void loop()
{
  // Make sure to call update as fast as possible
  pox.update();
}
