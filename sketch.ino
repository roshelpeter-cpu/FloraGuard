/**
 * @file sketch.ino
 * @brief FloraGuard environmental monitoring with servo ventilation.
 *
 * @details
 * Commit 5 adds PWM servo control for the automated nursery vent.
 */

#include "DHTesp.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>

// --------------------------------------------------
// Pin definitions
// --------------------------------------------------

const int DHT_PIN = 15;
const int LDR_PIN = 34;
const int DS18B20_PIN = 16;

const int OLED_SDA = 21;
const int OLED_SCL = 22;

const int SERVO_PIN = 27;

const int SCREEN_WIDTH = 128;
const int SCREEN_HEIGHT = 64;
const int OLED_ADDRESS = 0x3C;

// --------------------------------------------------
// Sensor objects
// --------------------------------------------------

DHTesp dht;

OneWire oneWire(DS18B20_PIN);

DallasTemperature outdoorSensor(&oneWire);

// --------------------------------------------------
// Servo
// --------------------------------------------------

Servo ventServo;

/**
 * @brief Current vent opening percentage.
 */
int ventPercent = 0;

// --------------------------------------------------
// OLED
// --------------------------------------------------

Adafruit_SSD1306 display(
    SCREEN_WIDTH,
    SCREEN_HEIGHT,
    &Wire,
    -1
);

// --------------------------------------------------
// Setup
// --------------------------------------------------

void setup()
{
  Serial.begin(115200);

  // DHT22
  dht.setup(DHT_PIN, DHTesp::DHT22);

  // LDR
  pinMode(LDR_PIN, INPUT);

  // DS18B20
  outdoorSensor.begin();

  // OLED
  Wire.begin(OLED_SDA, OLED_SCL);

  if (!display.begin(
          SSD1306_SWITCHCAPVCC,
          OLED_ADDRESS))
  {
    Serial.println("OLED ERROR: Display not found");

    while (true)
    {
    }
  }

  // Servo
  ventServo.attach(SERVO_PIN);

  // Start with vent closed
  ventServo.write(0);
  ventPercent = 0;

  // Startup display
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  display.setCursor(0, 0);
  display.println("FLORAGUARD");

  display.setCursor(0, 15);
  display.println("System Starting...");

  display.setCursor(0, 30);
  display.println("Vent: 0%");

  display.display();

  Serial.println();
  Serial.println("================================");
  Serial.println("FLORAGUARD - SERVO TEST");
  Serial.println("================================");
}

// --------------------------------------------------
// OLED update
// --------------------------------------------------

void updateDisplay(
    float indoorTemp,
    float humidity,
    float outdoorTemp,
    int lightPercent)
{
  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  display.setCursor(0, 0);
  display.println("FLORAGUARD");

  display.setCursor(0, 12);
  display.print("IN : ");
  display.print(indoorTemp, 1);
  display.println(" C");

  display.setCursor(0, 23);
  display.print("HUM: ");
  display.print(humidity, 1);
  display.println(" %");

  display.setCursor(0, 34);
  display.print("OUT: ");
  display.print(outdoorTemp, 1);
  display.println(" C");

  display.setCursor(0, 45);
  display.print("LGT: ");
  display.print(lightPercent);
  display.println(" %");

  display.setCursor(0, 56);
  display.print("VENT: ");
  display.print(ventPercent);
  display.println("%");

  display.display();
}

// --------------------------------------------------
// Main loop
// --------------------------------------------------

void loop()
{
  // ==================================================
  // DHT22
  // ==================================================

  TempAndHumidity dhtData =
      dht.getTempAndHumidity();

  float indoorTemperature =
      dhtData.temperature;

  float humidity =
      dhtData.humidity;

  if (dht.getStatus() != 0)
  {
    Serial.print("DHT22 ERROR: ");
    Serial.println(dht.getStatusString());
  }
  else
  {
    Serial.print("Indoor Temperature: ");
    Serial.print(indoorTemperature, 1);
    Serial.println(" C");

    Serial.print("Indoor Humidity: ");
    Serial.print(humidity, 1);
    Serial.println(" %");
  }

  // ==================================================
  // LDR
  // ==================================================

  int lightRaw =
      analogRead(LDR_PIN);

  int lightPercent =
      map(lightRaw, 0, 4095, 0, 100);

  lightPercent =
      constrain(lightPercent, 0, 100);

  Serial.print("Light Level: ");
  Serial.print(lightPercent);
  Serial.println(" %");

  // ==================================================
  // DS18B20
  // ==================================================

  outdoorSensor.requestTemperatures();

  float outdoorTemperature =
      outdoorSensor.getTempCByIndex(0);

  if (outdoorTemperature ==
          DEVICE_DISCONNECTED_C ||
      outdoorTemperature < -55.0 ||
      outdoorTemperature > 125.0)
  {
    Serial.println(
        "DS18B20 ERROR: Sensor unavailable");

    outdoorTemperature = 0.0;
  }
  else
  {
    Serial.print("Outdoor Temperature: ");
    Serial.print(outdoorTemperature, 1);
    Serial.println(" C");
  }

  // ==================================================
  // SERVO TEST
  // ==================================================

  // For this development stage, keep the vent
  // at 50% open.
  ventPercent = 75;

  int servoAngle =
      map(ventPercent, 0, 100, 0, 180);

  ventServo.write(servoAngle);

  Serial.print("Vent Position: ");
  Serial.print(ventPercent);
  Serial.println(" %");

  // ==================================================
  // OLED
  // ==================================================

  updateDisplay(
      indoorTemperature,
      humidity,
      outdoorTemperature,
      lightPercent);

  Serial.println("--------------------------------");

  delay(2500);
}