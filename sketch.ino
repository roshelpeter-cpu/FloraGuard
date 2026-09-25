// ============================================================
// FloraGuard
// Automated Commercial Micro-Climate Nursery
//
// Current Features:
// 1. DHT22 temperature & humidity
// 2. LDR light sensing
// 3. DS18B20 outdoor temperature
// 4. I2C OLED display
// 5. Servo ventilation
// 6. Buzzer alerts
// 7. Grow light control
// 8. Status LED
//
// IMPORTANT:
// No delay() is used.
// Timing is handled using millis().
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

// Sensors
const int DHT_PIN = 15;
const int LDR_PIN = 34;
const int DS18B20_PIN = 16;

// OLED
const int OLED_SDA = 21;
const int OLED_SCL = 22;

// Servo
const int SERVO_PIN = 27;

// Buzzer
const int BUZZER_PIN = 25;

// LEDs
const int GROW_LED_1 = 23;
const int GROW_LED_2 = 19;
const int STATUS_LED = 18;


// ============================================================
// OLED
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
// SENSOR VARIABLES
// ============================================================

float indoorTemperature = NAN;
float humidity = NAN;
float outdoorTemperature = NAN;

int ldrValue = 0;
int lightPercentage = 0;


// ============================================================
// SERVO VARIABLES
// ============================================================

int ventPosition = 50;


// ============================================================
// TIMING VARIABLES
// ============================================================

// DHT22
unsigned long lastDHTRead = 0;
const unsigned long DHT_INTERVAL = 2500;

// LDR
unsigned long lastLDRRead = 0;
const unsigned long LDR_INTERVAL = 250;

// DS18B20
unsigned long lastDSRequest = 0;
unsigned long dsConversionStart = 0;

const unsigned long DS_INTERVAL = 2000;
const unsigned long DS_CONVERSION_TIME = 800;

bool dsConversionRunning = false;

// OLED
unsigned long lastOLEDUpdate = 0;
const unsigned long OLED_INTERVAL = 500;

// LED status
unsigned long lastStatusBlink = 0;
const unsigned long STATUS_BLINK_INTERVAL = 500;

bool statusLEDState = false;

// Buzzer
unsigned long buzzerStartTime = 0;
unsigned long lastBuzzerEvent = 0;

const unsigned long BUZZER_INTERVAL = 1500;
const unsigned long BUZZER_DURATION = 200;

bool buzzerActive = false;


// ============================================================
// STARTUP BUZZER
// ============================================================

int startupBeepStep = 0;

unsigned long startupBeepTimer = 0;

bool startupComplete = false;


// ============================================================
// SENSOR STATUS
// ============================================================

bool dhtValid = false;
bool outdoorValid = false;


// ============================================================
// SETUP
// ============================================================

void setup() {

  Serial.begin(115200);

  Serial.println();
  Serial.println("========================================");
  Serial.println(" FloraGuard Nursery System");
  Serial.println(" Starting...");
  Serial.println("========================================");


  // ----------------------------------------------------------
  // DHT22
  // ----------------------------------------------------------

  dht.setup(
    DHT_PIN,
    DHTesp::DHT22
  );

  Serial.println("DHT22 initialized");


  // ----------------------------------------------------------
  // LDR
  // ----------------------------------------------------------

  pinMode(
    LDR_PIN,
    INPUT
  );

  Serial.println("LDR initialized");


  // ----------------------------------------------------------
  // DS18B20
  // ----------------------------------------------------------

  outdoorSensor.begin();

  outdoorSensor.setWaitForConversion(false);

  Serial.println("DS18B20 initialized");


  // ----------------------------------------------------------
  // OLED
  // ----------------------------------------------------------

  Wire.begin(
    OLED_SDA,
    OLED_SCL
  );

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
  }


  // ----------------------------------------------------------
  // SERVO
  // ----------------------------------------------------------

  ventServo.attach(
    SERVO_PIN
  );

  ventPosition = 50;

  ventServo.write(
    ventPosition
  );

  Serial.println("Servo initialized");
  Serial.println("Vent position: 50%");


  // ----------------------------------------------------------
  // BUZZER
  // ----------------------------------------------------------

  pinMode(
    BUZZER_PIN,
    OUTPUT
  );

  noTone(
    BUZZER_PIN
  );

  Serial.println("Buzzer initialized");


  // ----------------------------------------------------------
  // LEDS
  // ----------------------------------------------------------

  pinMode(
    GROW_LED_1,
    OUTPUT
  );

  pinMode(
    GROW_LED_2,
    OUTPUT
  );

  pinMode(
    STATUS_LED,
    OUTPUT
  );


  // Start all LEDs OFF
  digitalWrite(
    GROW_LED_1,
    LOW
  );

  digitalWrite(
    GROW_LED_2,
    LOW
  );

  digitalWrite(
    STATUS_LED,
    LOW
  );


  Serial.println("LEDs initialized");


  // ----------------------------------------------------------
  // START DS18B20 CONVERSION
  // ----------------------------------------------------------

  outdoorSensor.requestTemperatures();

  lastDSRequest = millis();

  dsConversionStart = millis();

  dsConversionRunning = true;


  // ----------------------------------------------------------
  // STARTUP BUZZER
  // ----------------------------------------------------------

  startupBeepTimer = millis();

  Serial.println("Startup sequence running...");
}


// ============================================================
// READ DHT22
// ============================================================

void readDHT22() {

  TempAndHumidity data =
    dht.getTempAndHumidity();


  indoorTemperature =
    data.temperature;

  humidity =
    data.humidity;


  // Check sensor values
  if (
    isnan(indoorTemperature) ||
    isnan(humidity)
  ) {

    dhtValid = false;

    Serial.println(
      "ERROR: DHT22 reading invalid"
    );

  } else {

    dhtValid = true;

    Serial.println();
    Serial.println("--- DHT22 ---");

    Serial.print(
      "Indoor Temperature: "
    );

    Serial.print(
      indoorTemperature
    );

    Serial.println(" C");


    Serial.print(
      "Humidity: "
    );

    Serial.print(
      humidity
    );

    Serial.println(" %");
  }
}


// ============================================================
// READ LDR
// ============================================================

void readLDR() {

  ldrValue =
    analogRead(
      LDR_PIN
    );


  lightPercentage =
    map(
      ldrValue,
      0,
      4095,
      0,
      100
    );


  lightPercentage =
    constrain(
      lightPercentage,
      0,
      100
    );


  Serial.println();
  Serial.println("--- LDR ---");

  Serial.print(
    "ADC Value: "
  );

  Serial.println(
    ldrValue
  );


  Serial.print(
    "Light Level: "
  );

  Serial.print(
    lightPercentage
  );

  Serial.println(" %");
}


// ============================================================
// DS18B20
// NON-BLOCKING READING
// ============================================================

void handleDS18B20() {

  unsigned long currentMillis =
    millis();


  // ----------------------------------------------------------
  // Start a new conversion
  // ----------------------------------------------------------

  if (
    !dsConversionRunning &&
    currentMillis - lastDSRequest >= DS_INTERVAL
  ) {

    outdoorSensor.requestTemperatures();

    dsConversionStart =
      currentMillis;

    lastDSRequest =
      currentMillis;

    dsConversionRunning =
      true;
  }


  // ----------------------------------------------------------
  // Check if conversion is complete
  // ----------------------------------------------------------

  if (
    dsConversionRunning &&
    currentMillis - dsConversionStart >=
      DS_CONVERSION_TIME
  ) {

    outdoorTemperature =
      outdoorSensor.getTempCByIndex(0);


    if (
      outdoorTemperature == DEVICE_DISCONNECTED_C ||
      outdoorTemperature < -55 ||
      outdoorTemperature > 125
    ) {

      outdoorValid = false;

      Serial.println(
        "ERROR: DS18B20 reading invalid"
      );

    } else {

      outdoorValid = true;

      Serial.println();
      Serial.println("--- DS18B20 ---");

      Serial.print(
        "Outdoor Temperature: "
      );

      Serial.print(
        outdoorTemperature
      );

      Serial.println(" C");
    }


    dsConversionRunning =
      false;
  }
}


// ============================================================
// GROW LIGHT CONTROL
// ============================================================
//
// Light level:
//
// 0 - 30%
// BOTH grow LEDs ON
//
// 30 - 60%
// ONE grow LED ON
//
// 60 - 100%
// BOTH grow LEDs OFF
//
// ============================================================

void controlGrowLights() {

  if (lightPercentage < 30) {

    // Very dark
    // Full grow lighting

    digitalWrite(
      GROW_LED_1,
      HIGH
    );

    digitalWrite(
      GROW_LED_2,
      HIGH
    );


    Serial.println(
      "Grow Light: FULL"
    );
  }

  else if (lightPercentage < 60) {

    // Medium light
    // Half grow lighting

    digitalWrite(
      GROW_LED_1,
      HIGH
    );

    digitalWrite(
      GROW_LED_2,
      LOW
    );


    Serial.println(
      "Grow Light: HALF"
    );
  }

  else {

    // Sufficient natural light

    digitalWrite(
      GROW_LED_1,
      LOW
    );

    digitalWrite(
      GROW_LED_2,
      LOW
    );


    Serial.println(
      "Grow Light: OFF"
    );
  }
}


// ============================================================
// VENTILATION CONTROL
// ============================================================

void controlVentilation() {

  if (!dhtValid) {

    return;
  }


  // ----------------------------------------------------------
  // High temperature
  // ----------------------------------------------------------

  if (
    indoorTemperature >= 30.0
  ) {

    ventPosition =
      100;
  }


  // ----------------------------------------------------------
  // Warm
  // ----------------------------------------------------------

  else if (
    indoorTemperature >= 27.0
  ) {

    ventPosition =
      75;
  }


  // ----------------------------------------------------------
  // Normal
  // ----------------------------------------------------------

  else {

    ventPosition =
      50;
  }


  ventServo.write(
    ventPosition
  );


  Serial.println();
  Serial.println("--- Ventilation ---");

  Serial.print(
    "Vent Position: "
  );

  Serial.print(
    ventPosition
  );

  Serial.println("%");
}


// ============================================================
// BUZZER CONTROL
// NON-BLOCKING
// ============================================================

void controlBuzzer() {

  unsigned long currentMillis =
    millis();


  bool warning =
    false;


  // High temperature
  if (
    dhtValid &&
    indoorTemperature >= 32.0
  ) {

    warning = true;
  }


  // High humidity
  if (
    dhtValid &&
    humidity >= 85.0
  ) {

    warning = true;
  }


  // ----------------------------------------------------------
  // Warning active
  // ----------------------------------------------------------

  if (warning) {

    if (
      !buzzerActive &&
      currentMillis - lastBuzzerEvent >=
        BUZZER_INTERVAL
    ) {

      tone(
        BUZZER_PIN,
        2000
      );

      buzzerActive = true;

      buzzerStartTime =
        currentMillis;

      lastBuzzerEvent =
        currentMillis;


      Serial.println(
        "BUZZER: WARNING"
      );
    }


    if (
      buzzerActive &&
      currentMillis - buzzerStartTime >=
        BUZZER_DURATION
    ) {

      noTone(
        BUZZER_PIN
      );

      buzzerActive = false;
    }
  }

  else {

    if (buzzerActive) {

      noTone(
        BUZZER_PIN
      );

      buzzerActive = false;
    }
  }
}


// ============================================================
// STATUS LED
// ============================================================
//
// Normal:
// ON
//
// Warning:
// Blink
//
// Sensor fault:
// Fast blink
//
// ============================================================

void updateStatusLED() {

  unsigned long currentMillis =
    millis();


  bool sensorFault =
    !dhtValid;


  bool warning =
    dhtValid &&
    (
      indoorTemperature >= 32.0 ||
      humidity >= 85.0
    );


  // ----------------------------------------------------------
  // Sensor fault
  // ----------------------------------------------------------

  if (sensorFault) {

    if (
      currentMillis - lastStatusBlink >= 150
    ) {

      lastStatusBlink =
        currentMillis;

      statusLEDState =
        !statusLEDState;

      digitalWrite(
        STATUS_LED,
        statusLEDState
      );
    }

    return;
  }


  // ----------------------------------------------------------
  // Warning
  // ----------------------------------------------------------

  if (warning) {

    if (
      currentMillis - lastStatusBlink >=
        STATUS_BLINK_INTERVAL
    ) {

      lastStatusBlink =
        currentMillis;

      statusLEDState =
        !statusLEDState;

      digitalWrite(
        STATUS_LED,
        statusLEDState
      );
    }

    return;
  }


  // ----------------------------------------------------------
  // Normal
  // ----------------------------------------------------------

  statusLEDState =
    true;

  digitalWrite(
    STATUS_LED,
    HIGH
  );
}


// ============================================================
// STARTUP BUZZER
// NON-BLOCKING
// ============================================================

void handleStartupBuzzer() {

  unsigned long currentMillis =
    millis();


  if (startupComplete) {

    return;
  }


  // ----------------------------------------------------------
  // Step 0
  // First beep
  // ----------------------------------------------------------

  if (
    startupBeepStep == 0
  ) {

    tone(
      BUZZER_PIN,
      1000
    );

    startupBeepTimer =
      currentMillis;

    startupBeepStep =
      1;

    return;
  }


  // ----------------------------------------------------------
  // Step 1
  // End first beep
  // ----------------------------------------------------------

  if (
    startupBeepStep == 1 &&
    currentMillis - startupBeepTimer >= 300
  ) {

    noTone(
      BUZZER_PIN
    );

    startupBeepTimer =
      currentMillis;

    startupBeepStep =
      2;

    return;
  }


  // ----------------------------------------------------------
  // Step 2
  // Wait before second beep
  // ----------------------------------------------------------

  if (
    startupBeepStep == 2 &&
    currentMillis - startupBeepTimer >= 200
  ) {

    tone(
      BUZZER_PIN,
      1500
    );

    startupBeepTimer =
      currentMillis;

    startupBeepStep =
      3;

    return;
  }


  // ----------------------------------------------------------
  // Step 3
  // End second beep
  // ----------------------------------------------------------

  if (
    startupBeepStep == 3 &&
    currentMillis - startupBeepTimer >= 300
  ) {

    noTone(
      BUZZER_PIN
    );

    startupComplete =
      true;

    Serial.println(
      "Startup sequence complete"
    );
  }
}


// ============================================================
// OLED DISPLAY
// ============================================================

void updateOLED() {

  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(
    SSD1306_WHITE
  );


  // ----------------------------------------------------------
  // Title
  // ----------------------------------------------------------

  display.setCursor(
    0,
    0
  );

  display.println(
    "FloraGuard"
  );


  // ----------------------------------------------------------
  // Indoor temperature
  // ----------------------------------------------------------

  display.setCursor(
    0,
    12
  );

  display.print(
    "Indoor: "
  );

  if (dhtValid) {

    display.print(
      indoorTemperature,
      1
    );

    display.println(
      " C"
    );

  } else {

    display.println(
      "ERROR"
    );
  }


  // ----------------------------------------------------------
  // Humidity
  // ----------------------------------------------------------

  display.setCursor(
    0,
    22
  );

  display.print(
    "Humidity: "
  );

  if (dhtValid) {

    display.print(
      humidity,
      1
    );

    display.println(
      " %"
    );

  } else {

    display.println(
      "ERROR"
    );
  }


  // ----------------------------------------------------------
  // Outdoor temperature
  // ----------------------------------------------------------

  display.setCursor(
    0,
    32
  );

  display.print(
    "Outdoor: "
  );

  if (outdoorValid) {

    display.print(
      outdoorTemperature,
      1
    );

    display.println(
      " C"
    );

  } else {

    display.println(
      "ERROR"
    );
  }


  // ----------------------------------------------------------
  // Light
  // ----------------------------------------------------------

  display.setCursor(
    0,
    42
  );

  display.print(
    "Light: "
  );

  display.print(
    lightPercentage
  );

  display.println(
    " %"
  );


  // ----------------------------------------------------------
  // Vent
  // ----------------------------------------------------------

  display.setCursor(
    0,
    52
  );

  display.print(
    "Vent: "
  );

  display.print(
    ventPosition
  );

  display.println(
    "%"
  );


  display.display();
}


// ============================================================
// MAIN LOOP
// ============================================================

void loop() {

  unsigned long currentMillis =
    millis();


  // ----------------------------------------------------------
  // STARTUP BUZZER
  // ----------------------------------------------------------

  handleStartupBuzzer();


  // ----------------------------------------------------------
  // DHT22
  // ----------------------------------------------------------

  if (
    currentMillis - lastDHTRead >=
      DHT_INTERVAL
  ) {

    lastDHTRead =
      currentMillis;

    readDHT22();
  }


  // ----------------------------------------------------------
  // LDR
  // ----------------------------------------------------------

  if (
    currentMillis - lastLDRRead >=
      LDR_INTERVAL
  ) {

    lastLDRRead =
      currentMillis;

    readLDR();
  }


  // ----------------------------------------------------------
  // DS18B20
  // ----------------------------------------------------------

  handleDS18B20();


  // ----------------------------------------------------------
  // Grow lights
  // ----------------------------------------------------------

  controlGrowLights();


  // ----------------------------------------------------------
  // Ventilation
  // ----------------------------------------------------------

  controlVentilation();


  // ----------------------------------------------------------
  // Buzzer
  // ----------------------------------------------------------

  controlBuzzer();


  // ----------------------------------------------------------
  // Status LED
  // ----------------------------------------------------------

  updateStatusLED();


  // ----------------------------------------------------------
  // OLED
  // ----------------------------------------------------------

  if (
    currentMillis - lastOLEDUpdate >=
      OLED_INTERVAL
  ) {

    lastOLEDUpdate =
      currentMillis;

    updateOLED();
  }
}