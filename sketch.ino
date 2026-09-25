#include "DHTesp.h"

const int DHT_PIN = 15;

DHTesp dht;

void setup()
{
  Serial.begin(115200);

  dht.setup(DHT_PIN, DHTesp::DHT22);

  Serial.println();
  Serial.println("================================");
  Serial.println("DHT22 TEST");
  Serial.println("================================");
  Serial.print("Sampling period: ");
  Serial.print(dht.getMinimumSamplingPeriod());
  Serial.println(" ms");
}

void loop()
{
  TempAndHumidity data = dht.getTempAndHumidity();

  if (dht.getStatus() != 0)
  {
    Serial.print("DHT ERROR: ");
    Serial.println(dht.getStatusString());

    Serial.print("STATUS CODE: ");
    Serial.println(dht.getStatus());
  }
  else
  {
    Serial.print("Temperature: ");
    Serial.print(data.temperature, 1);
    Serial.println(" C");

    Serial.print("Humidity: ");
    Serial.print(data.humidity, 1);
    Serial.println(" %");

    Serial.println("----------------------------");
  }

  delay(2500);
}