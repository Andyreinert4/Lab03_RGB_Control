// *************************************************************************
// Title        : Lab 03 RGB Control
// File Name    : 'main.cpp'
// Target MCU   : Espressif ESP32 (Doit DevKit Version 1)
//
// EGRT 390     : Demonstration of RGB Control
//
// Revision History:
// When         Who         Description of change
// -----------  ----------- -----------------------
// 12-MAR-2025  [A.Reinert] initial commit
// 12-MAR-2025 [A.Reinert] added RGB BLINK
// 12-MAR-2025 [A.Reinert] added notes 
// 12-MAR-2025 [A.Reinert] Final Commit
// *************************************************************************

// Include Files
// *************************************************************************
#include <Arduino.h>                // Standard Arduino library
#include <Wire.h>                   // I2C library (ADC)
#include <ADS1X15.h>                // ADS1115 library (ADC)
#include "fonts/Roboto_Mono_14.h"   // Font 1 for OLED display
#include "fonts/Mountains_of_Christmas_Regular_12.h" // Font 2 for OLED display
#include "fonts/Ultra_Regular_16.h"  // Font 3 for OLED display
#include <SSD1306Wire.h>            // OLED display library
#include <stdint.h>                 // Standard integer library
#include <FastLED.h>                // FastLED library for RGB LED
#include <debounce.h>               // Debounce library for push button

// Globals
// *************************************************************************
const uint8_t LED = 15;                // GPIO pin for the LED
const uint16_t BLINK_INTERVAL = 1000;  // Blink interval in milliseconds
bool ledState = false;                 // LED state
unsigned long ledBlinkTime = 0;        // Time for the next blink

ADS1115 ADS(0x48);                     // Create an ADS1115 object with the default I2C address 0x48
SSD1306Wire display(0x3c, SDA, SCL);   // OLED display

enum Position               // Text position on the OLED display         
{ TOP, 
  MIDDLE, 
  BOTTOM, 
  UNKNOWN 
}; 
const uint8_t TOP_Y = 0;              // Y position for the top line
const uint8_t MIDDLE_Y = 24;          // Y position for the middle line
const uint8_t BOTTOM_Y = 48;          // Y position for the bottom line

const uint8_t Roboto_Mono_14 []       // Font 1 for the OLED display
{
  0x0E, 0x0E, 0x0E, 0x0E, 0x0E, 0x00, 0x0E
};

String inputString = "";           // A string to hold incoming serial data
bool stringComplete = false;       // Whether the string is complete
 
String topText = "Top Line";        // Text for the top line
String middleText = "Middle Line";  // Text for the middle line
String bottomText = "Bottom Line";  // Text for the bottom line

// Define the number of LEDs and the data pin
const uint8_t NUM_LEDS = 1;         // Number of LEDs
const uint8_t DATA_PIN = 16;        // Data pin for the LED strip
CRGB leds[NUM_LEDS];                // Define the LED strip

// Define the colors
enum Color                          // Enum for the LED colors
{ Red, 
  Green, 
  Blue, 
  Yellow, 
  Cyan, 
  Purple, 
  Orange 
};
Color currentColor = Red; 

// Brightness control
uint8_t brightness = 128; // Initial brightness level (0-255)
bool increasing = true;   // Direction of brightness change

// Push button
const uint8_t BUTTON_PIN = 17;    // GPIO pin for the push button
enum LEDState { OFF, ON, BLINK };  // LED state modes
LEDState ledStateMode = OFF;       // Initial LED state mode
unsigned long buttonDebounceTime = 0;  // Time for the next button debounce
const uint16_t DEBOUNCE_DELAY = 50;    // Debounce delay in milliseconds

// Function Prototypes
// *************************************************************************
void updateLEDState();               // Update the LED state
CRGB getColorFromEnum(Color color);  // Get the CRGB color from the enum
void displayAllText();               // Display all text on the OLED display
void processSerialCommand();         // Process the serial command
void checkButtonState();             // Check the button state
Position getPositionFromString(const String &posStr);  // Get the position from the string

// Setup Code
// *************************************************************************
void setup() 
{
  Serial.begin(115200);           // Initialize serial communication at 115200 baud rate
  Serial.println("ADS testing");  // Print a message to the serial monitor
  inputString.reserve(128);       // Reserve memory for the input string
  Wire.begin();                   // Initialize I2C communication
  ADS.begin();                    // Initialize ADS1115
  pinMode(LED, OUTPUT);           // Initialize the LED pin as an output

  ADS.setGain(1);                // Set gain to 1 (±4.096V)
  ADS.setMode(0);                // Set to continuous conversion mode
  ADS.getValue();                // Read the initial value

  // OLED Display Setup
  inputString.reserve(128);      // Reserve memory for the input string
  display.init();                // Initialize the OLED display
  display.displayOn();           // Turn on the display
  display.clear();               // Clear the display
  display.setTextAlignment(TEXT_ALIGN_LEFT);  // Set text alignment
  display.display();             // Display the cleared buffer

  // RGB LED Setup
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS); // Initialize LED
  FastLED.show();                // Display the initial LED state

  // Button Setup
  pinMode(BUTTON_PIN, INPUT_PULLUP); 

  // Print the initial message
  Serial.println("\nText Display Example"); 
  Serial.println("Enter commands like: top:Hello World");
  Serial.println("Valid positions are: top, middle, bottom");
}

// Main program
// *************************************************************************
void loop()
{
  // Heartbeat LED
  if (millis() - ledBlinkTime >= BLINK_INTERVAL) 
  {
    ledState = !ledState;                    // Toggle the LED state
    digitalWrite(LED, ledState);             // Set the LED state
    ledBlinkTime = millis();                 // Update the blink time
  }

  // Read the value from the ADS1115
  int16_t value0 = ADS.readADC(0);           // Read the value from ADS1115
  float voltage0 = ADS.toVoltage(2) / 2;     // Convert the value to voltage
  Serial.print("Analog0: ");                 // Print the channel name
  Serial.print(value0);                      // Print the raw value
  Serial.print("\t ");                       // Print a tab character
  Serial.println(value0 * voltage0, 3);      // Print the voltage value

  // Display the value on the OLED display
  displayAllText();

  // Process serial input
  if (stringComplete)
  {
    processSerialCommand();    // Process the serial command
    stringComplete = false;    // Reset the flag
    inputString = "";          // Clear the input string
  }

  // Check button state
  checkButtonState();   

  // Update RGB LED state
  updateLEDState();
}

// Function Definitions
// *************************************************************************
Position getPositionFromString(const String &posStr) // Get the position from the string
{
  if (posStr == "top")  
  {
    return TOP;
  }
  else if (posStr == "middle")
  {
    return MIDDLE;
  }
  else if (posStr == "bottom")
  {
    return BOTTOM;
  }
  else
  {
    return UNKNOWN;
  }
}

// processSerialCommand
// *************************************************************************
void processSerialCommand() 
{
  inputString.trim();  // Remove leading and trailing whitespace
  int8_t colonIndex = inputString.indexOf(':'); // Find the colon index
  if (colonIndex == -1)                         // Check if colon is missing
  {
    Serial.println("Error: Missing Text. Use format: Position:Text or Color:ColorName");
    return;
  }
  String command = inputString.substring(0, colonIndex);
  String value = inputString.substring(colonIndex + 1);
  
  if (command == "top" || command == "middle" || command == "bottom")
  {
    Position pos = getPositionFromString(command);
    switch (pos)
    {
      case TOP:
        topText = value;
        Serial.println(value);
        Serial.println("Top Line Updated");
        break;
      case MIDDLE:
        middleText = value;
        Serial.println(value);
        Serial.println("Middle Line Updated");
        break;
      case BOTTOM:
        bottomText = value;
        Serial.println(value);
        Serial.println("Bottom Line Updated");
        break;
      case UNKNOWN:
        Serial.println("Error: Unknown Position. Valid positions are: top, middle, bottom");
        break;
    }
  }
  else if (command == "color")
  {
    if (value == "Red") currentColor = Red;
    else if (value == "Green") currentColor = Green;
    else if (value == "Blue") currentColor = Blue;
    else if (value == "Yellow") currentColor = Yellow;
    else if (value == "Cyan") currentColor = Cyan;
    else if (value == "Purple") currentColor = Purple;
    else if (value == "Orange") currentColor = Orange;
    else Serial.println("Error: Unknown Color. Valid colors are: Red, Green, Blue, Yellow, Cyan, Purple, Orange");
    Serial.print("Color Updated to: ");
    Serial.println(value);
  }
  else
  {
    Serial.println("Error: Unknown Command. Valid commands are: top, middle, bottom, color");
  }
}

// displayAllText
// *************************************************************************
void displayAllText()
{
  display.clear();                       // Clear the display
  display.setFont(Roboto_Mono_14);       // Top line font
  display.drawString(0, TOP_Y, topText); // Display the top line
  display.setFont(Ultra_Regular_16);     // Middle line font
  display.drawString(0, MIDDLE_Y, middleText);  // Display the middle line
  display.setFont(Mountains_of_Christmas_Regular_12); // Bottom line font
  display.drawString(0, BOTTOM_Y, bottomText);  // Display the bottom line
  display.display();                     // Display the buffer
}

// serialEvent
// *************************************************************************
void serialEvent()
{
  while (Serial.available())
  {
    char inChar = (char) Serial.read();
    if (inChar != '\n' && inChar != '\r')
    {
      inputString += inChar;
    }
    if (inChar == '\n')
    {
      stringComplete = true;
    }
  }
}

// checkButtonState
// *************************************************************************
void checkButtonState()
{
  static bool lastButtonState = HIGH;
  bool buttonState = digitalRead(BUTTON_PIN);
  if (buttonState != lastButtonState && millis() - buttonDebounceTime > DEBOUNCE_DELAY)
  {
    buttonDebounceTime = millis();
    if (buttonState == LOW)
    {
      ledStateMode = static_cast<LEDState>((ledStateMode + 1) % 3);
      if (ledStateMode == ON) {
        currentColor = static_cast<Color>((currentColor + 1) % 7); // Cycle through colors
      }
      Serial.print("LED State: ");
      Serial.println(ledStateMode == OFF ? "OFF" : (ledStateMode == ON ? "ON" : "BLINK"));
    }
    lastButtonState = buttonState;
  }
}

// getColorFromEnum
// *************************************************************************
CRGB getColorFromEnum(Color color)
{
  switch (color)
  {
    case Red: return CRGB::Red;
    case Green: return CRGB::Green;
    case Blue: return CRGB::Blue;
    case Yellow: return CRGB::Yellow;
    case Cyan: return CRGB::Cyan;
    case Purple: return CRGB::Purple;
    case Orange: return CRGB::Orange;
    default: return CRGB::Black;
  }
}

// updateLEDState
// *************************************************************************
void updateLEDState()
{
  static unsigned long previousMillis = 0;
  unsigned long currentMillis = millis();

  switch (ledStateMode)
  {
    case OFF:
      leds[0] = CRGB::Black;
      break;
    case ON:
      leds[0] = getColorFromEnum(currentColor);
      break;
    case BLINK:
      if (currentMillis - previousMillis >= BLINK_INTERVAL) {
        previousMillis = currentMillis;
        leds[0] = leds[0] == CRGB::Black ? getColorFromEnum(currentColor) : CRGB::Black;
      }
      break;
  }
  FastLED.show();
}