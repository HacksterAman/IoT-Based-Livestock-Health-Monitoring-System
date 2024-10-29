#include <Wire.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"

MAX30105 particleSensor;

// Buffer size and data type update
#define BUFFER_SIZE 100
uint32_t irBuffer[BUFFER_SIZE];  // Use uint32_t for the IR buffer
uint32_t redBuffer[BUFFER_SIZE]; // Use uint32_t for the red buffer

int32_t bufferLength;  // data length
int32_t spo2;          // SPO2 value
int8_t validSPO2;      // indicator to show if the SPO2 calculation is valid
int32_t heartRate;     // heart rate value
int8_t validHeartRate; // indicator to show if the heart rate calculation is valid

void setup()
{
  Serial.begin(115200);
  // Initialize sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST))
  {
    Serial.println("MAX30105 was not found. Please check wiring/power.");
    while (1)
      ;
  }

  particleSensor.setup();                    // Configure sensor with default settings
  particleSensor.setPulseAmplitudeRed(0x0A); // Low Red LED amplitude
  particleSensor.setPulseAmplitudeGreen(0);  // Turn off Green LED
  particleSensor.enableDIETEMPRDY();         // Enable the temperature ready interrupt
}

void loop()
{
  // Collect samples
  bufferLength = BUFFER_SIZE; // buffer length

  // Read the first 100 samples
  for (byte i = 0; i < bufferLength; i++)
  {
    while (particleSensor.available() == false)
    {
      particleSensor.check(); // Check for new data
    }
    redBuffer[i] = particleSensor.getRed();
    irBuffer[i] = particleSensor.getIR();
    particleSensor.nextSample(); // Move to next sample
  }

  // Calculate heart rate and SpO2
  maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);

  // Display results
  Serial.print("Heart Rate: ");
  Serial.print(heartRate);
  Serial.print(" Valid HR: ");
  Serial.println(validHeartRate);

  Serial.print("SpO2: ");
  Serial.print(spo2);
  Serial.print(" Valid SpO2: ");
  Serial.println(validSPO2);

  delay(1000); // Delay for readability
}
