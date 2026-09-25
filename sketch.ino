// ============================================================
// FloraGuard - Automated Commercial Micro-Climate Nursery
// Current Development Version
//
// Features implemented:
// 1. DHT22 temperature & humidity
// 2. LDR light sensing
// 3. DS18B20 outdoor temperature
// 4. I2C OLED environmental display
// 5. Servo ventilation control
// 6. Buzzer alerts
//
// LEDs are temporarily NOT used.
// ============================================================

#include <DHTesp.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>

// ============================================================
// PIN DEFINITIONS
// ============================================================

// DHT22
const int DHT_PIN = 15;

// LDR
const int LDR_PIN = 34;

// DS18B20
const int DS18B20_PIN = 16;

// OLED
const int OLED_SDA = 21;
const int OLED_SCL = 22;

// Servo
const int SERVO_PIN = 27;

// Buzzer
const int BUZZER_PIN = 25;


// ============================================================
// OLED SETTINGS
// ============================================================

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  OLED_RESET
);


// ============================================================
// SENSOR OBJECTS
// ============================================================

DHTesp dht;

OneWire oneWire(DS18B20_PIN);
DallasTemperature outdoorSensor(&oneWire);

Servo ventServo;


// ============================================================
// VARIABLES
// ============================================================

float indoorTemperature = 0.0;
float humidity = 0.0;
float outdoorTemperature = 0.0;

int ldrValue = 0;
int lightPercentage = 0;

int ventPosition = 50;


// ============================================================
// BUZZER SETTINGS
// ============================================================

bool buzzerTestDone = false;


// ============================================================
// SETUP
// ============================================================

void setup() {

  Serial.begin(115200);

  Serial.println();
  Serial.println("========================================");
  Serial.println(" FloraGuard Nursery System");
  Serial.println(" System Starting...");
  Serial.println("========================================");


  // ----------------------------------------------------------
  // DHT22
  // ----------------------------------------------------------

  dht.setup(DHT_PIN, DHTesp::DHT22);

  Serial.println("DHT22 initialized");


  // ----------------------------------------------------------
  // LDR
  // ----------------------------------------------------------

  pinMode(LDR_PIN, INPUT);

  Serial.println("LDR initialized");


  // ----------------------------------------------------------
  // DS18B20
  // ----------------------------------------------------------

  outdoorSensor.begin();

  Serial.println("DS18B20 initialized");


  // ----------------------------------------------------------
  // OLED
  // ----------------------------------------------------------

  Wire.begin(OLED_SDA, OLED_SCL);

  if (!display.begin(
        SSD1306_SWITCHCAPVCC,
        0x3C
      )) {

    Serial.println("OLED initialization failed!");

  } else {

    Serial.println("OLED initialized");

    display.clearDisplay();

    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    display.setCursor(0, 0);
    display.println("FloraGuard");

    display.setCursor(0, 15);
    display.println("System Starting...");

    display.display();

    delay(1500);
  }


  // ----------------------------------------------------------
  // SERVO
  // ----------------------------------------------------------

  ventServo.attach(SERVO_PIN);

  ventServo.write(ventPosition);

  Serial.println("Servo initialized");
  Serial.println("Vent position: 50%");


  // ----------------------------------------------------------
  // BUZZER
  // ----------------------------------------------------------

  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(BUZZER_PIN, LOW);

  Serial.println("Buzzer initialized");


  // ----------------------------------------------------------
  // STARTUP BUZZER
  // ----------------------------------------------------------

  Serial.println("Buzzer startup test");

  tone(BUZZER_PIN, 1000);

  delay(300);

  noTone(BUZZER_PIN);

  delay(200);


  tone(BUZZER_PIN, 1500);

  delay(300);

  noTone(BUZZER_PIN);


  Serial.println("Buzzer test complete");

  Serial.println();
  Serial.println("========================================");
  Serial.println(" FloraGuard Ready");
  Serial.println("========================================");
}


// ============================================================
// READ DHT22
// ============================================================

void readDHT22() {

  TempAndHumidity data = dht.getTempAndHumidity();

  indoorTemperature = data.temperature;
  humidity = data.humidity;


  Serial.println();
  Serial.println("--- DHT22 ---");

  Serial.print("Indoor Temperature: ");
  Serial.print(indoorTemperature);
  Serial.println(" C");

  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println(" %");
}


// ============================================================
// READ LDR
// ============================================================

void readLDR() {

  ldrValue = analogRead(LDR_PIN);


  // Convert ADC value to percentage
  lightPercentage = map(
    ldrValue,
    0,
    4095,
    0,
    100
  );


  lightPercentage = constrain(
    lightPercentage,
    0,
    100
  );


  Serial.println();
  Serial.println("--- LDR ---");

  Serial.print("ADC Value: ");
  Serial.println(ldrValue);

  Serial.print("Light Level: ");
  Serial.print(lightPercentage);
  Serial.println(" %");
}


// ============================================================
// READ DS18B20
// ============================================================

void readOutdoorTemperature() {

  outdoorSensor.requestTemperatures();

  outdoorTemperature =
    outdoorSensor.getTempCByIndex(0);


  Serial.println();
  Serial.println("--- DS18B20 ---");

  Serial.print("Outdoor Temperature: ");
  Serial.print(outdoorTemperature);
  Serial.println(" C");
}


// ============================================================
// UPDATE OLED
// ============================================================

void updateOLED() {

  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);


  // Title
  display.setCursor(0, 0);
  display.println("FloraGuard");


  // Indoor temperature
  display.setCursor(0, 12);
  display.print("Indoor: ");
  display.print(indoorTemperature, 1);
  display.println(" C");


  // Humidity
  display.setCursor(0, 22);
  display.print("Humidity: ");
  display.print(humidity, 1);
  display.println(" %");


  // Outdoor temperature
  display.setCursor(0, 32);
  display.print("Outdoor: ");
  display.print(outdoorTemperature, 1);
  display.println(" C");


  // Light
  display.setCursor(0, 42);
  display.print("Light: ");
  display.print(lightPercentage);
  display.println(" %");


  // Vent
  display.setCursor(0, 52);
  display.print("Vent: ");
  display.print(ventPosition);
  display.println("%");


  display.display();
}


// ============================================================
// VENTILATION CONTROL
// ============================================================

void controlVentilation() {

  // Basic development-stage ventilation logic
  //
  // If indoor temperature is high,
  // open the vent.
  //
  // Otherwise keep the vent at 50%.

  if (indoorTemperature >= 30.0) {

    ventPosition = 100;

  }

  else if (indoorTemperature >= 27.0) {

    ventPosition = 75;

  }

  else {

    ventPosition = 50;
  }


  ventServo.write(ventPosition);


  Serial.println();
  Serial.println("--- Ventilation ---");

  Serial.print("Vent Position: ");
  Serial.print(ventPosition);
  Serial.println("%");
}


// ============================================================
// BUZZER ALERT
// ============================================================

void checkBuzzer() {

  // High temperature warning

  if (indoorTemperature >= 32.0) {

    Serial.println("WARNING: High indoor temperature!");

    tone(BUZZER_PIN, 2000);

    delay(200);

    noTone(BUZZER_PIN);

  }

  // Very high humidity warning

  else if (humidity >= 85.0) {

    Serial.println("WARNING: High humidity!");

    tone(BUZZER_PIN, 1500);

    delay(200);

    noTone(BUZZER_PIN);
  }

  else {

    noTone(BUZZER_PIN);
  }
}


// ============================================================
// MAIN LOOP
// ============================================================

void loop() {

  // ----------------------------------------------------------
  // Read sensors
  // ----------------------------------------------------------

  readDHT22();

  readLDR();

  readOutdoorTemperature();


  // ----------------------------------------------------------
  // Control ventilation
  // ----------------------------------------------------------

  controlVentilation();


  // ----------------------------------------------------------
  // Update OLED
  // ----------------------------------------------------------

  updateOLED();


  // ----------------------------------------------------------
  // Check alerts
  // ----------------------------------------------------------

  checkBuzzer();


  // ----------------------------------------------------------
  // Wait before next sensor cycle
  // ----------------------------------------------------------

  delay(2500);
}