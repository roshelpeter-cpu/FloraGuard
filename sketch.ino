// ============================================================
// FloraGuard
// Automated Commercial Micro-Climate Nursery
//
// Commit 8:
// - Sensor validation
// - Safety / Error mode
// - Safe vent posture
// - OLED fault indication
// - State LED blinking in safety mode
//
// Current hardware:
// DHT22   -> GPIO15
// LDR     -> GPIO34
// DS18B20 -> GPIO16
// OLED    -> SDA GPIO21, SCL GPIO22
// Servo   -> GPIO27
// Buzzer  -> GPIO25
// Grow LED -> GPIO19
// State LED -> GPIO23
//
// IMPORTANT:
// No delay() is used. Timing uses millis().
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

const int DHT_PIN = 15;
const int LDR_PIN = 34;
const int DS18B20_PIN = 16;

const int OLED_SDA = 21;
const int OLED_SCL = 22;

const int SERVO_PIN = 27;
const int BUZZER_PIN = 25;

const int GROW_LED = 19;
const int STATE_LED = 23;

// ============================================================
// SYSTEM MODES
// ============================================================

enum SystemMode {
  AUTOMATION_MODE,
  MANUAL_MODE,
  EMERGENCY_MODE
};

SystemMode currentMode = AUTOMATION_MODE;

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
// SENSOR VALUES
// ============================================================

float indoorTemperature = NAN;
float humidity = NAN;
float outdoorTemperature = NAN;

int ldrValue = 0;
int lightPercentage = 0;

// ============================================================
// SENSOR VALIDATION
// ============================================================

bool dhtValid = false;
bool outdoorValid = false;

bool dhtReadAttempted = false;
bool outdoorReadAttempted = false;

// ============================================================
// SAFETY MODE
// ============================================================

bool safetyMode = false;

// The predefined safe physical posture.
// 100% means the ventilation vent is fully open.
const int SAFE_VENT_POSITION = 100;

// ============================================================
// SERVO
// ============================================================

int ventPosition = 50;

// ============================================================
// TIMING
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

// State LED
unsigned long lastStateBlink = 0;
const unsigned long STATE_BLINK_INTERVAL = 300;
bool stateLEDState = false;

// Buzzer
unsigned long buzzerStartTime = 0;
unsigned long lastBuzzerEvent = 0;

const unsigned long BUZZER_INTERVAL = 1500;
const unsigned long BUZZER_DURATION = 200;

bool buzzerActive = false;

// Startup buzzer
int startupBeepStep = 0;
unsigned long startupBeepTimer = 0;
bool startupComplete = false;

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
  outdoorSensor.setWaitForConversion(false);

  Serial.println("DS18B20 initialized");

  // ----------------------------------------------------------
  // OLED
  // ----------------------------------------------------------

  Wire.begin(OLED_SDA, OLED_SCL);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {

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

  ventServo.attach(SERVO_PIN);

  ventPosition = 50;

  ventServo.write(ventPosition);

  Serial.println("Servo initialized");
  Serial.println("Vent position: 50%");

  // ----------------------------------------------------------
  // BUZZER
  // ----------------------------------------------------------

  pinMode(BUZZER_PIN, OUTPUT);
  noTone(BUZZER_PIN);

  Serial.println("Buzzer initialized");

  // ----------------------------------------------------------
  // LEDS
  // ----------------------------------------------------------

  pinMode(GROW_LED, OUTPUT);
  pinMode(STATE_LED, OUTPUT);

  digitalWrite(GROW_LED, LOW);
  digitalWrite(STATE_LED, LOW);

  Serial.println("Grow Light GPIO19 initialized");
  Serial.println("State Light GPIO23 initialized");

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
  Serial.println();
  Serial.println("SYSTEM MODE: AUTOMATION");
  Serial.println();
  Serial.println("Serial commands:");
  Serial.println("A = Automation");
  Serial.println("M = Manual Override");
  Serial.println("E = Emergency");
}

// ============================================================
// READ DHT22
// ============================================================

void readDHT22() {

  TempAndHumidity data = dht.getTempAndHumidity();

  indoorTemperature = data.temperature;
  humidity = data.humidity;

  dhtReadAttempted = true;

  // DHT22 normal operating range:
  // Temperature: -40 to 80 C
  // Humidity: 0 to 100 %
  if (
    isnan(indoorTemperature) ||
    isnan(humidity) ||
    indoorTemperature < -40.0 ||
    indoorTemperature > 80.0 ||
    humidity < 0.0 ||
    humidity > 100.0
  ) {

    dhtValid = false;

    Serial.println();
    Serial.println("========================================");
    Serial.println("ERROR: DHT22 INVALID READING");
    Serial.println("========================================");

  } else {

    dhtValid = true;

    Serial.println();
    Serial.println("--- DHT22 ---");

    Serial.print("Indoor Temperature: ");
    Serial.print(indoorTemperature);
    Serial.println(" C");

    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");
  }
}

// ============================================================
// READ LDR
// ============================================================

void readLDR() {

  ldrValue = analogRead(LDR_PIN);

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
// DS18B20
// NON-BLOCKING
// ============================================================

void handleDS18B20() {

  unsigned long currentMillis = millis();

  // Start a new conversion
  if (
    !dsConversionRunning &&
    currentMillis - lastDSRequest >= DS_INTERVAL
  ) {

    outdoorSensor.requestTemperatures();

    dsConversionStart = currentMillis;
    lastDSRequest = currentMillis;
    dsConversionRunning = true;
  }

  // Read completed conversion
  if (
    dsConversionRunning &&
    currentMillis - dsConversionStart >= DS_CONVERSION_TIME
  ) {

    outdoorTemperature =
      outdoorSensor.getTempCByIndex(0);

    outdoorReadAttempted = true;

    if (
      outdoorTemperature == DEVICE_DISCONNECTED_C ||
      outdoorTemperature < -55.0 ||
      outdoorTemperature > 125.0 ||
      isnan(outdoorTemperature)
    ) {

      outdoorValid = false;

      Serial.println();
      Serial.println("========================================");
      Serial.println("ERROR: DS18B20 INVALID READING");
      Serial.println("========================================");

    } else {

      outdoorValid = true;

      Serial.println();
      Serial.println("--- DS18B20 ---");

      Serial.print("Outdoor Temperature: ");
      Serial.print(outdoorTemperature);
      Serial.println(" C");
    }

    dsConversionRunning = false;
  }
}

// ============================================================
// SAFETY / SENSOR VALIDATION
// ============================================================
//
// If a sensor becomes invalid after a reading has been
// attempted, the system enters Safety/Error mode.
//
// Safety response:
// 1. Enter EMERGENCY_MODE
// 2. Latch safetyMode
// 3. Open vent to predefined safe position
// 4. OLED identifies failed sensor
// 5. Buzzer gives warning pulses
// 6. GPIO23 state LED blinks
//
// Safety mode remains active until a later operator-reset
// mechanism is implemented.
// ============================================================

void handleSafetyMode() {

  bool dhtFault =
    dhtReadAttempted && !dhtValid;

  bool outdoorFault =
    outdoorReadAttempted && !outdoorValid;

  if (
    (dhtFault || outdoorFault) &&
    !safetyMode
  ) {

    safetyMode = true;
    currentMode = EMERGENCY_MODE;

    // Predefined safe physical posture
    ventPosition = SAFE_VENT_POSITION;
    ventServo.write(SAFE_VENT_POSITION);

    Serial.println();
    Serial.println("========================================");
    Serial.println(" SAFETY / ERROR MODE ACTIVATED");
    Serial.println("========================================");

    if (dhtFault) {
      Serial.println("FAULT: DHT22 SENSOR");
    }

    if (outdoorFault) {
      Serial.println("FAULT: DS18B20 SENSOR");
    }

    Serial.print("Safe posture: VENT ");
    Serial.print(SAFE_VENT_POSITION);
    Serial.println("% OPEN");

    Serial.println("State LED: BLINKING");
    Serial.println("Buzzer: WARNING");
    Serial.println("Operator intervention required");
    Serial.println("========================================");
  }

  // Keep the physical safe posture while safety is active.
  if (safetyMode) {

    ventPosition = SAFE_VENT_POSITION;
    ventServo.write(SAFE_VENT_POSITION);
  }
}

// ============================================================
// GROW LIGHT CONTROL
// ============================================================
//
// GPIO19 is kept exactly as the existing working grow-light
// implementation.
//
// <30%  -> ON
// 30-59% -> ON
// >=60% -> OFF
// ============================================================

void controlGrowLight() {

  if (lightPercentage < 30) {

    digitalWrite(GROW_LED, HIGH);

    Serial.println("Grow Light: ON");

  } else if (lightPercentage < 60) {

    digitalWrite(GROW_LED, HIGH);

    Serial.println("Grow Light: ON");

  } else {

    digitalWrite(GROW_LED, LOW);

    Serial.println("Grow Light: OFF");
  }
}

// ============================================================
// VENTILATION CONTROL
// ============================================================

void controlVentilation() {

  // Safety has highest priority.
  if (safetyMode) {

    ventPosition = SAFE_VENT_POSITION;
    ventServo.write(SAFE_VENT_POSITION);

    return;
  }

  if (!dhtValid) {
    return;
  }

  if (indoorTemperature >= 30.0) {

    ventPosition = 100;

  } else if (indoorTemperature >= 27.0) {

    ventPosition = 75;

  } else {

    ventPosition = 50;
  }

  // Only automation mode changes the servo automatically.
  if (currentMode == AUTOMATION_MODE) {

    ventServo.write(ventPosition);
  }

  Serial.println();
  Serial.println("--- Ventilation ---");

  Serial.print("Vent Position: ");
  Serial.print(ventPosition);
  Serial.println("%");
}

// ============================================================
// BUZZER CONTROL
// NON-BLOCKING
// ============================================================

void controlBuzzer() {

  unsigned long currentMillis = millis();

  bool warning = false;

  // High temperature warning
  if (
    dhtValid &&
    indoorTemperature >= 32.0
  ) {
    warning = true;
  }

  // High humidity warning
  if (
    dhtValid &&
    humidity >= 85.0
  ) {
    warning = true;
  }

  // Safety / emergency warning
  if (
    currentMode == EMERGENCY_MODE ||
    safetyMode
  ) {
    warning = true;
  }

  if (warning) {

    if (
      !buzzerActive &&
      currentMillis - lastBuzzerEvent >= BUZZER_INTERVAL
    ) {

      tone(BUZZER_PIN, 2000);

      buzzerActive = true;
      buzzerStartTime = currentMillis;
      lastBuzzerEvent = currentMillis;

      Serial.println("BUZZER: WARNING");
    }

    if (
      buzzerActive &&
      currentMillis - buzzerStartTime >= BUZZER_DURATION
    ) {

      noTone(BUZZER_PIN);
      buzzerActive = false;
    }

  } else {

    if (buzzerActive) {

      noTone(BUZZER_PIN);
      buzzerActive = false;
    }
  }
}

// ============================================================
// SYSTEM STATE LIGHT - GPIO23
// ============================================================
//
// AUTOMATION:
// OFF
//
// MANUAL:
// ON
//
// EMERGENCY / SAFETY:
// BLINK
// ============================================================

void updateStateLight() {

  unsigned long currentMillis = millis();

  // Automation
  if (currentMode == AUTOMATION_MODE) {

    digitalWrite(STATE_LED, LOW);
    stateLEDState = false;

    return;
  }

  // Manual Override
  if (currentMode == MANUAL_MODE) {

    digitalWrite(STATE_LED, HIGH);
    stateLEDState = true;

    return;
  }

  // Emergency / Safety
  if (currentMode == EMERGENCY_MODE) {

    if (
      currentMillis - lastStateBlink >=
      STATE_BLINK_INTERVAL
    ) {

      lastStateBlink = currentMillis;

      stateLEDState = !stateLEDState;

      digitalWrite(
        STATE_LED,
        stateLEDState
      );
    }
  }
}

// ============================================================
// SERIAL MODE CONTROL
// ============================================================
//
// A = Automation
// M = Manual Override
// E = Emergency
//
// IMPORTANT:
// If automatic Safety Mode has already been triggered,
// command A does NOT clear safetyMode yet.
// A proper operator reset will be added in the next mode
// / manual-override stage.
// ============================================================

void handleSerialCommands() {

  if (!Serial.available()) {
    return;
  }

  char command = Serial.read();

  // ----------------------------------------------------------
  // AUTOMATION
  // ----------------------------------------------------------

  if (
    command == 'A' ||
    command == 'a'
  ) {

    if (safetyMode) {

      Serial.println();
      Serial.println("AUTOMATION BLOCKED");
      Serial.println("SYSTEM IS IN SAFETY MODE");
      Serial.println("OPERATOR INTERVENTION REQUIRED");

      return;
    }

    currentMode = AUTOMATION_MODE;

    Serial.println();
    Serial.println("MODE CHANGED: AUTOMATION");
    Serial.println("State Light: OFF");
  }

  // ----------------------------------------------------------
  // MANUAL
  // ----------------------------------------------------------

  else if (
    command == 'M' ||
    command == 'm'
  ) {

    if (safetyMode) {

      Serial.println();
      Serial.println("MANUAL MODE BLOCKED");
      Serial.println("SYSTEM IS IN SAFETY MODE");
      Serial.println("OPERATOR INTERVENTION REQUIRED");

      return;
    }

    currentMode = MANUAL_MODE;

    Serial.println();
    Serial.println("MODE CHANGED: MANUAL OVERRIDE");
    Serial.println("State Light: ON");
  }

  // ----------------------------------------------------------
  // EMERGENCY
  // ----------------------------------------------------------

  else if (
    command == 'E' ||
    command == 'e'
  ) {

    currentMode = EMERGENCY_MODE;

    Serial.println();
    Serial.println("MODE CHANGED: EMERGENCY");
    Serial.println("State Light: BLINKING");
  }
}

// ============================================================
// STARTUP BUZZER
// NON-BLOCKING
// ============================================================

void handleStartupBuzzer() {

  unsigned long currentMillis = millis();

  if (startupComplete) {
    return;
  }

  if (startupBeepStep == 0) {

    tone(BUZZER_PIN, 1000);

    startupBeepTimer = currentMillis;
    startupBeepStep = 1;

    return;
  }

  if (
    startupBeepStep == 1 &&
    currentMillis - startupBeepTimer >= 300
  ) {

    noTone(BUZZER_PIN);

    startupBeepTimer = currentMillis;
    startupBeepStep = 2;

    return;
  }

  if (
    startupBeepStep == 2 &&
    currentMillis - startupBeepTimer >= 200
  ) {

    tone(BUZZER_PIN, 1500);

    startupBeepTimer = currentMillis;
    startupBeepStep = 3;

    return;
  }

  if (
    startupBeepStep == 3 &&
    currentMillis - startupBeepTimer >= 300
  ) {

    noTone(BUZZER_PIN);

    startupComplete = true;

    Serial.println("Startup sequence complete");
  }
}

// ============================================================
// OLED DISPLAY
// ============================================================

void updateOLED() {

  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // ----------------------------------------------------------
  // SAFETY SCREEN
  // ----------------------------------------------------------

  if (safetyMode) {

    display.setCursor(0, 0);
    display.println("!! SENSOR FAULT !!");

    display.setCursor(0, 12);

    if (
      dhtReadAttempted &&
      !dhtValid
    ) {
      display.println("FAILED: DHT22");
    }

    else if (
      outdoorReadAttempted &&
      !outdoorValid
    ) {
      display.println("FAILED: DS18B20");
    }

    else {
      display.println("FAILED: SENSOR");
    }

    display.setCursor(0, 24);
    display.print("SAFE VENT: ");
    display.print(SAFE_VENT_POSITION);
    display.println("%");

    display.setCursor(0, 36);
    display.println("MODE: SAFETY");

    display.setCursor(0, 48);
    display.println("CHECK SENSOR");

    display.setCursor(0, 58);
    display.println("OPERATOR REQUIRED");

    display.display();

    return;
  }

  // ----------------------------------------------------------
  // NORMAL SCREEN
  // ----------------------------------------------------------

  display.setCursor(0, 0);
  display.println("FloraGuard");

  // Indoor temperature
  display.setCursor(0, 12);
  display.print("Indoor: ");

  if (dhtValid) {

    display.print(indoorTemperature, 1);
    display.println(" C");

  } else {

    display.println("ERROR");
  }

  // Humidity
  display.setCursor(0, 22);
  display.print("Humidity: ");

  if (dhtValid) {

    display.print(humidity, 1);
    display.println(" %");

  } else {

    display.println("ERROR");
  }

  // Outdoor temperature
  display.setCursor(0, 32);
  display.print("Outdoor: ");

  if (outdoorValid) {

    display.print(outdoorTemperature, 1);
    display.println(" C");

  } else {

    display.println("ERROR");
  }

  // Light
  display.setCursor(0, 42);
  display.print("Light: ");
  display.print(lightPercentage);
  display.println(" %");

  // Mode
  display.setCursor(0, 52);

  if (currentMode == AUTOMATION_MODE) {

    display.print("AUTO");

  } else if (currentMode == MANUAL_MODE) {

    display.print("MANUAL");

  } else {

    display.print("EMERGENCY");
  }

  display.display();
}

// ============================================================
// MAIN LOOP
// ============================================================

void loop() {

  unsigned long currentMillis = millis();

  // ----------------------------------------------------------
  // Serial commands
  // ----------------------------------------------------------

  handleSerialCommands();

  // ----------------------------------------------------------
  // Startup buzzer
  // ----------------------------------------------------------

  handleStartupBuzzer();

  // ----------------------------------------------------------
  // DHT22
  // ----------------------------------------------------------

  if (
    currentMillis - lastDHTRead >= DHT_INTERVAL
  ) {

    lastDHTRead = currentMillis;

    readDHT22();
  }

  // ----------------------------------------------------------
  // LDR
  // ----------------------------------------------------------

  if (
    currentMillis - lastLDRRead >= LDR_INTERVAL
  ) {

    lastLDRRead = currentMillis;

    readLDR();
  }

  // ----------------------------------------------------------
  // DS18B20
  // ----------------------------------------------------------

  handleDS18B20();

  // ----------------------------------------------------------
  // SAFETY VALIDATION
  // ----------------------------------------------------------

  handleSafetyMode();

  // ----------------------------------------------------------
  // Grow Light - GPIO19
  // ----------------------------------------------------------

  controlGrowLight();

  // ----------------------------------------------------------
  // Ventilation
  // ----------------------------------------------------------

  controlVentilation();

  // ----------------------------------------------------------
  // Buzzer
  // ----------------------------------------------------------

  controlBuzzer();

  // ----------------------------------------------------------
  // State Light - GPIO23
  // ----------------------------------------------------------

  updateStateLight();

  // ----------------------------------------------------------
  // OLED
  // ----------------------------------------------------------

  if (
    currentMillis - lastOLEDUpdate >= OLED_INTERVAL
  ) {

    lastOLEDUpdate = currentMillis;

    updateOLED();
  }
}
