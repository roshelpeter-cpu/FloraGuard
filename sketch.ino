/**
 * @file sketch.ino
 * @brief FloraGuard sensor development - DHT22, LDR and DS18B20.
 *
 * @details
 * Commit 3 adds outdoor temperature monitoring using
 * a DS18B20 digital temperature sensor.
 */

#include "DHTesp.h"
#include <OneWire.h>
#include <DallasTemperature.h>

// --------------------------------------------------
// Pin definitions
// --------------------------------------------------

/** @brief DHT22 data pin. */
const int DHT_PIN = 15;

/** @brief LDR analogue output pin. */
const int LDR_PIN = 34;

/** @brief DS18B20 data pin. */
const int DS18B20_PIN = 16;

// --------------------------------------------------
// Sensor objects
// --------------------------------------------------

/** @brief DHT22 sensor object. */
DHTesp dht;

/** @brief OneWire communication bus. */
OneWire oneWire(DS18B20_PIN);

/** @brief DS18B20 temperature sensor object. */
DallasTemperature outdoorSensor(&oneWire);

// --------------------------------------------------
// Setup
// --------------------------------------------------

/**
 * @brief Initialises sensors and serial communication.
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

  Serial.println();
  Serial.println("================================");
  Serial.println("FLORAGUARD - SENSOR TEST");
  Serial.println("DHT22 + LDR + DS18B20");
  Serial.println("================================");
}

// --------------------------------------------------
// Main loop
// --------------------------------------------------

/**
 * @brief Reads and displays all environmental sensors.
 */
void loop()
{
  // ==================================================
  // DHT22 - Indoor temperature and humidity
  // ==================================================

  TempAndHumidity dhtData = dht.getTempAndHumidity();

  if (dht.getStatus() != 0)
  {
    Serial.print("DHT22 ERROR: ");
    Serial.println(dht.getStatusString());
  }
  else
  {
    Serial.print("Indoor Temperature: ");
    Serial.print(dhtData.temperature, 1);
    Serial.println(" C");

    Serial.print("Indoor Humidity: ");
    Serial.print(dhtData.humidity, 1);
    Serial.println(" %");
  }

  // ==================================================
  // LDR - Light level
  // ==================================================

  int lightRaw = analogRead(LDR_PIN);

  int lightPercent = map(lightRaw, 0, 4095, 0, 100);
  lightPercent = constrain(lightPercent, 0, 100);

  Serial.print("LDR Raw ADC: ");
  Serial.println(lightRaw);

  Serial.print("Light Level: ");
  Serial.print(lightPercent);
  Serial.println(" %");

  // ==================================================
  // DS18B20 - Outdoor temperature
  // ==================================================

  outdoorSensor.requestTemperatures();

  float outdoorTemperature =
      outdoorSensor.getTempCByIndex(0);

  if (outdoorTemperature == DEVICE_DISCONNECTED_C ||
      outdoorTemperature < -55.0 ||
      outdoorTemperature > 125.0)
  {
    Serial.println("DS18B20 ERROR: Sensor unavailable");
  }
  else
  {
    Serial.print("Outdoor Temperature: ");
    Serial.print(outdoorTemperature, 1);
    Serial.println(" C");
  }

  // ==================================================

  Serial.println("--------------------------------");

  // Temporary development delay.
  // This will be removed when we implement
  // non-blocking timing later.
  delay(2500);
}