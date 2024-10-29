#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

// Define OLED display width and height
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Declare the display object with I2C address 0x3C
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void setup()
{
  // Start the display
  if (!display.begin(0x3C))
  {
    Serial.println(F("SH110X OLED not found"));
    while (1)
      ; // Halt if display not found
  }

  display.clearDisplay();
  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setTextColor(SH110X_WHITE); // White text
  display.setCursor(0, 0);            // Start at top-left corner
  display.println(F("Hello, World!"));
  display.display();
}

void loop()
{
  // Add more code here if you want to update the display
}
