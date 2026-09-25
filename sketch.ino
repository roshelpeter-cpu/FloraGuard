/**
 * @file sketch.ino
 * @brief FloraGuard sensor development - DHT22 and LDR.
 *
 * @details
 * Commit 2 adds analogue light monitoring using the LDR.
 * The DHT22 continues to provide indoor temperature and humidity.
 */

#include "DHTesp.h"

// --------------------------------------------------
// Pin definitions
// --------------------------------------------------

/** @brief DHT22 data pin. */
const int DHT_PIN = 15;

/** @brief LDR analogue output pin. */
const int LDR_PIN = 34;

// --------------------------------------------------
// Sensor objects
// --------------------------------------------------

/** @brief DHT22 sensor object. */
DHTesp dht;

// --------------------------------------------------
// Setup
// --------------------------------------------------

/**
 * @brief Initialises the sensors and serial communication.
 */
void setup()
{
  Serial.begin(115200);

  // Initialise DHT22
  dht.setup(DHT_PIN, DHTesp::DHT22);

  // Configure LDR as analogue input
  pinMode(LDR_PIN, INPUT);

  Serial.println();
  Serial.println("================================");
  Serial.println("FLORAGUARD - SENSOR TEST");
  Serial.println("DHT22 + LDR");
  Serial.println("================================");
}

// --------------------------------------------------
// Main loop
// --------------------------------------------------

/**
 * @brief Reads and displays DHT22 and LDR values.
 */
void loop()
{
  // -----------------------------------------------
  // DHT22
  // -----------------------------------------------

  TempAndHumidity dhtData = dht.getTempAndHumidity();

  if (dht.getStatus() != 0)
  {
    Serial.print("DHT22 ERROR: ");
    Serial.println(dht.getStatusString());
  }
  else
  {
    Serial.print("Temperature: ");
    Serial.print(dhtData.temperature, 1);
    Serial.println(" C");

    Serial.print("Humidity: ");
    Serial.print(dhtData.humidity, 1);
    Serial.println(" %");
  }

  // -----------------------------------------------
  // LDR
  // -----------------------------------------------

  int lightRaw = analogRead(LDR_PIN);

  Serial.print("LDR Raw ADC: ");
  Serial.println(lightRaw);

  // Convert ADC reading into an approximate percentage.
  // Higher ADC value = more light in the current
  // Wokwi LDR configuration.
  int lightPercent = map(lightRaw, 0, 4095, 0, 100);

  lightPercent = constrain(lightPercent, 0, 100);

  Serial.print("Light Level: ");
  Serial.print(lightPercent);
  Serial.println(" %");

  Serial.println("--------------------------------");

  // DHT22 requires approximately 2 seconds between readings.
  delay(2500);
}