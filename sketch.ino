/**
 * @file sketch.ino
 * @brief FloraGuard sensor monitoring with I2C OLED display.
 *
 * @details
 * Commit 4 adds the SSD1306 I2C OLED display while
 * retaining DHT22, LDR and DS18B20 monitoring.
 */

#include "DHTesp.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --------------------------------------------------
// Pin definitions
// --------------------------------------------------

/** @brief DHT22 data pin. */
const int DHT_PIN = 15;

/** @brief LDR analogue output pin. */
const int LDR_PIN = 34;

/** @brief DS18B20 data pin. */
const int DS18B20_PIN = 16;

/** @brief OLED I2C SDA pin. */
const int OLED_SDA = 21;

/** @brief OLED I2C SCL pin. */
const int OLED_SCL = 22;

/** @brief OLED display width. */
const int SCREEN_WIDTH = 128;

/** @brief OLED display height. */
const int SCREEN_HEIGHT = 64;

/** @brief OLED I2C address. */
const int OLED_ADDRESS = 0x3C;

// --------------------------------------------------
// Sensor objects
// --------------------------------------------------

/** @brief DHT22 sensor object. */
DHTesp dht;

/** @brief OneWire communication bus. */
OneWire oneWire(DS18B20_PIN);

/** @brief DS18B20 sensor object. */
DallasTemperature outdoorSensor(&oneWire);

// --------------------------------------------------
// OLED object
// --------------------------------------------------

/**
 * @brief SSD1306 OLED display object.
 */
Adafruit_SSD1306 display(
    SCREEN_WIDTH,
    SCREEN_HEIGHT,
    &Wire,
    -1
);

// --------------------------------------------------
// Setup
// --------------------------------------------------

/**
 * @brief Initialises sensors, OLED and serial communication.
 */
void setup()
{
  Serial.begin(115200);

  // DHT22
  dht.setup(DHT_PIN, DHTesp::DHT22);

  // LDR
  pinMode(LDR_PIN, INPUT);

  // DS18B20
  outdoorSensor.begin();

  // OLED I2C
  Wire.begin(OLED_SDA, OLED_SCL);

  if (!display.begin(
          SSD1306_SWITCHCAPVCC,
          OLED_ADDRESS))
  {
    Serial.println("OLED ERROR: Display not found");

    while (true)
    {
      // Stop here if OLED initialisation fails.
    }
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("FLORAGUARD");
  display.println();
  display.println("System Starting...");
  display.display();

  Serial.println();
  Serial.println("================================");
  Serial.println("FLORAGUARD - OLED TEST");
  Serial.println("DHT22 + LDR + DS18B20");
  Serial.println("================================");
}

// --------------------------------------------------
// OLED update
// --------------------------------------------------

/**
 * @brief Updates the OLED with current sensor values.
 *
 * @param indoorTemp Indoor temperature in Celsius.
 * @param humidity Indoor relative humidity percentage.
 * @param outdoorTemp Outdoor temperature in Celsius.
 * @param lightPercent Estimated light percentage.
 */
void updateDisplay(
    float indoorTemp,
    float humidity,
    float outdoorTemp,
    int lightPercent)
{
  display.clearDisplay();

  display.setTextSize(1);

  display.setCursor(0, 0);
  display.println("FLORAGUARD");

  display.setCursor(0, 13);
  display.print("IN : ");
  display.print(indoorTemp, 1);
  display.println(" C");

  display.setCursor(0, 25);
  display.print("HUM: ");
  display.print(humidity, 1);
  display.println(" %");

  display.setCursor(0, 37);
  display.print("OUT: ");
  display.print(outdoorTemp, 1);
  display.println(" C");

  display.setCursor(0, 49);
  display.print("LGT: ");
  display.print(lightPercent);
  display.println(" %");

  display.display();
}

// --------------------------------------------------
// Main loop
// --------------------------------------------------

/**
 * @brief Reads sensors and updates the OLED display.
 */
void loop()
{
  // ==================================================
  // DHT22
  // ==================================================

  TempAndHumidity dhtData = dht.getTempAndHumidity();

  float indoorTemperature = dhtData.temperature;
  float humidity = dhtData.humidity;

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

  int lightRaw = analogRead(LDR_PIN);

  int lightPercent =
      map(lightRaw, 0, 4095, 0, 100);

  lightPercent =
      constrain(lightPercent, 0, 100);

  Serial.print("LDR Raw ADC: ");
  Serial.println(lightRaw);

  Serial.print("Light Level: ");
  Serial.print(lightPercent);
  Serial.println(" %");

  // ==================================================
  // DS18B20
  // ==================================================

  outdoorSensor.requestTemperatures();

  float outdoorTemperature =
      outdoorSensor.getTempCByIndex(0);

  if (outdoorTemperature == DEVICE_DISCONNECTED_C ||
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
  // OLED
  // ==================================================

  updateDisplay(
      indoorTemperature,
      humidity,
      outdoorTemperature,
      lightPercent);

  Serial.println("--------------------------------");

  // Temporary development delay.
  // Non-blocking timing will be implemented later.
  delay(2500);
}