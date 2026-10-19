
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>

// OLED SETTINGS
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32

Adafruit_SSD1306 display(
  SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1
);

// SENSOR PINS
#define SOIL_PIN A0
#define DHT_PIN 2
#define DHT_TYPE DHT11

// LED PINS
#define RED_LED 3
#define GREEN_LED 4
#define BLUE_LED 5
#define ALERT_LED 6

// SOIL CALIBRATION
// Replace these with your own readings
const int DRY_VALUE = 900;
const int WET_VALUE = 300;

// TIMING
const unsigned long SCREEN_INTERVAL = 10000;
const unsigned long SENSOR_INTERVAL = 2000;
const unsigned long FLASH_INTERVAL = 400;

DHT dht(DHT_PIN, DHT_TYPE);

int screenPage = 0;
int moisture = 0;

float temperature = NAN;
float humidity = NAN;

bool critical = false;
bool ledState = false;

unsigned long lastScreenChange = 0;
unsigned long lastSensorRead = 0;
unsigned long lastFlash = 0;

// RGB LED IS COMMON ANODE
void setColor(bool red, bool green, bool blue) {
  digitalWrite(RED_LED, red ? LOW : HIGH);
  digitalWrite(GREEN_LED, green ? LOW : HIGH);
  digitalWrite(BLUE_LED, blue ? LOW : HIGH);
}

// DISPLAY THE SELECTED READING
void updateDisplay() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);

  if (screenPage == 0) {
    display.println("SOIL MOISTURE");
    display.setTextSize(2);
    display.setCursor(0, 14);
    display.print(moisture);
    display.print("%");
  }
  else if (screenPage == 1) {
    display.println("TEMPERATURE");
    display.setTextSize(2);
    display.setCursor(0, 14);

    if (isnan(temperature)) {
      display.print("Error");
    } else {
      display.print(temperature, 1);
      display.print(" C");
    }
  }
  else {
    display.println("AIR HUMIDITY");
    display.setTextSize(2);
    display.setCursor(0, 14);

    if (isnan(humidity)) {
      display.print("Error");
    } else {
      display.print(humidity, 1);
      display.print("%");
    }
  }

  display.display();
}

void setup() {
  Serial.begin(115200);
  dht.begin();

  pinMode(SOIL_PIN, INPUT);

  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  pinMode(ALERT_LED, OUTPUT);

  digitalWrite(ALERT_LED, LOW);
  setColor(0, 0, 0);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED not found!");
    while (1);
  }

  display.clearDisplay();
  display.display();

  Serial.println("System Initialized!");
}

void loop() {
  unsigned long now = millis();

  // READ SENSORS EVERY 2 SECONDS
  if (lastSensorRead == 0 ||
      now - lastSensorRead >= SENSOR_INTERVAL) {

    lastSensorRead = now;

    int raw = analogRead(SOIL_PIN);

    if (DRY_VALUE != WET_VALUE) {
      moisture = map(
        raw, DRY_VALUE, WET_VALUE, 0, 100
      );
      moisture = constrain(moisture, 0, 100);
    }

    temperature = dht.readTemperature();
    humidity = dht.readHumidity();

    Serial.print("Raw Soil: ");
    Serial.println(raw);

    Serial.print("Moisture: ");
    Serial.print(moisture);
    Serial.println("%");

    Serial.print("Temperature: ");
    Serial.println(temperature);

    Serial.print("Humidity: ");
    Serial.println(humidity);

    // CHECK ALERT CONDITIONS
    critical = moisture < 30 ||
      (!isnan(temperature) &&
       (temperature > 30 || temperature < 20));

    if (critical) {
      setColor(1, 0, 0); // RED
    }
    else if (moisture < 40) {
      setColor(0, 0, 1); // BLUE
    }
    else {
      setColor(0, 1, 0); // GREEN
    }

    updateDisplay();
  }

  // FLASH ALERT LED
  if (critical) {
    if (now - lastFlash >= FLASH_INTERVAL) {
      lastFlash = now;
      ledState = !ledState;

      digitalWrite(
        ALERT_LED, ledState ? HIGH : LOW
      );
    }
  }
  else {
    ledState = false;
    digitalWrite(ALERT_LED, LOW);
  }

  // AUTOMATIC SCREEN SWITCHING
  if (now - lastScreenChange >= SCREEN_INTERVAL) {
    lastScreenChange = now;

    screenPage = (screenPage + 1) % 3;

    updateDisplay();
  }
}
