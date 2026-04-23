#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include <Adafruit_NeoPixel.h>
#include <DHT20.h>
DHT20 dht20;

#define MOISTURE_PIN P0_ADC
#define LIGHT_PIN P1_ADC
#define SERVO_PIN P14
Servo myServo;
#define NEOPIXEL_PIN 4
#define NUM_LEDS 25
Adafruit_NeoPixel leds(NUM_LEDS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

#define WET_THRESHOLD   2800
#define DRY_THRESHOLD   1200
#define NOISE_THRESHOLD 500
#define NUM_SAMPLES     20
#define TRIM_COUNT      5
#define SMOOTH_FACTOR   0.2

LiquidCrystal_I2C lcd(0x27, 16, 2);

bool lcdOK = false;
float smoothedRaw = -1; 
float smoothedLight = -1;
int servoPos = 0;
int servoDir = 1;
unsigned long lastServoMove = 0;

byte dropIcon[8] = {0x04, 0x04, 0x0A, 0x0A, 0x11, 0x11, 0x11, 0x0E};
byte barFull[8]  = {0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F};
byte sunIcon[8] = {0x00, 0x15, 0x0E, 0x1F, 0x0E, 0x15, 0x00, 0x00};

byte findLCD() {
  byte addrs[] = {0x27, 0x3F, 0x21, 0x20};
  for (int i = 0; i < 4; i++) {
    Wire.beginTransmission(addrs[i]);
    if (Wire.endTransmission() == 0) return addrs[i];
  }
  return 0;
}

void sortArray(int arr[], int size) {
  for (int i = 0; i < size - 1; i++) {
    for (int j = i + 1; j < size; j++) {
      if (arr[i] > arr[j]) {
        int tmp = arr[i];
        arr[i] = arr[j];
        arr[j] = tmp;
      }
    }
  }
}

int readSensorFiltered(float &stdDev) {
  int readings[NUM_SAMPLES];

  for (int i = 0; i < NUM_SAMPLES; i++) {
    readings[i] = analogRead(MOISTURE_PIN);
    delay(5);
  }
 
  sortArray(readings, NUM_SAMPLES);

  long total = 0;
  int count = NUM_SAMPLES - (TRIM_COUNT * 2);
  for (int i = TRIM_COUNT; i < NUM_SAMPLES - TRIM_COUNT; i++) {
    total += readings[i];
  }
  int avg = total / count;

  float sumSq = 0;
  for (int i = TRIM_COUNT; i < NUM_SAMPLES - TRIM_COUNT; i++) {
    float diff = readings[i] - avg;
    sumSq += diff * diff;
  }
  stdDev = sqrt(sumSq / count);
  
  return avg;
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== AIoT Farm ===");

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  myServo.setPeriodHertz(50);
  myServo.attach(SERVO_PIN, 500, 2400);
  myServo.write(0);

  leds.begin();
  leds.setBrightness(20);
  leds.clear();
  leds.show();

  analogReadResolution(12);  

  #if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
    analogSetAttenuation(ADC_ATTENDB_MAX);
  #else
    analogSetAttenuation(ADC_11db);
  #endif
  pinMode(MOISTURE_PIN, INPUT);
  
  for (int i = 0; i < 10; i++) {
    analogRead(MOISTURE_PIN);
    delay(10);
  }
  
  Wire.begin();
  dht20.begin();
  
  byte addr = findLCD();
  if (addr > 0) {
    lcd = LiquidCrystal_I2C(addr, 16, 2);
    lcd.init();
    lcd.backlight();
    lcd.createChar(0, dropIcon);
    lcd.createChar(1, barFull);
    lcd.createChar(2, sunIcon);
    lcd.setCursor(0, 0);
    lcd.print("AIoT Farm");
    lcd.setCursor(0, 1);
    lcdOK = true;
    Serial.println(addr, HEX);
  } else {
    Serial.println("Missing LCD");
  }
  delay(500);
  if (lcdOK) lcd.clear();
}

void loop() {
  float stdDev = 0;
  int rawValue = readSensorFiltered(stdDev);
  bool sensorConnected = true;
  if (stdDev > NOISE_THRESHOLD) {
    sensorConnected = false;
  }

  if (sensorConnected) {
    if (smoothedRaw < 0) {
      smoothedRaw = rawValue;
    } else {
      smoothedRaw = smoothedRaw * (1.0 - SMOOTH_FACTOR) + rawValue * SMOOTH_FACTOR;
    }
  }

  int lightRaw = analogRead(LIGHT_PIN);
  if (smoothedLight < 0) {
    smoothedLight = lightRaw;
  } else {
    smoothedLight = smoothedLight * (1.0 - SMOOTH_FACTOR) + lightRaw * SMOOTH_FACTOR;
  }
  int lightPercent = map((int)smoothedLight, 0, 4095, 0, 100);
  lightPercent = constrain(lightPercent, 0, 100);

  int displayRaw = (int)smoothedRaw;
  int percent = map(displayRaw, 0, 4095, 0, 100);
  percent = constrain(percent, 0, 100);
  String status;

  if (!sensorConnected) {
    leds.clear();
  } else if (displayRaw > WET_THRESHOLD) {
    for (int i = 0; i < NUM_LEDS; i++) leds.setPixelColor(i, 0, 100, 255);
  } else if (displayRaw < DRY_THRESHOLD) {
    for (int i = 0; i < NUM_LEDS; i++) leds.setPixelColor(i, 255, 0, 0);
  } else {
    for (int i = 0; i < NUM_LEDS; i++) leds.setPixelColor(i, 255, 200, 0);
  }
  leds.show();

  dht20.read();
  float temp = dht20.getTemperature();
  float humi = dht20.getHumidity();
  
  if (!sensorConnected) {
    status = "No Sensor";
    percent = 0;
  } else if (displayRaw > WET_THRESHOLD) {
    status = "Wet   ";
  } else if (displayRaw < DRY_THRESHOLD) {
    status = "Dry   ";
  } else {
    status = "Normal   ";
  }

if (millis() - lastServoMove > 1000) {
    if (servoPos == 0) {
      servoPos = 180;
    } else {
      servoPos = 0;
    }
    myServo.write(servoPos);
    lastServoMove = millis();
  }

if (lcdOK) {
    lcd.setCursor(0, 0);
    if (!sensorConnected) {
      lcd.print("! Error  !");
      lcd.setCursor(0, 1);
      lcd.print("Check wires     ");
    } else {
      lcd.write(byte(0));
      if (percent < 10) lcd.print(" ");
      if (percent < 100) lcd.print(" ");
      lcd.print(percent);
      lcd.print("%  ");
      lcd.write(byte(2));
      if (lightPercent < 10) lcd.print(" ");
      if (lightPercent < 100) lcd.print(" ");
      lcd.print(lightPercent);
      lcd.print("%   ");
      lcd.setCursor(0, 1);
      lcd.print((int)temp);
      lcd.print("C  ");
      lcd.print((int)humi);
      lcd.print("%RH      ");
    }
  }

  Serial.print("Soil: ");
  Serial.print(percent);
  Serial.print("% | Light: ");
  Serial.print(lightPercent);
  Serial.print("% | Temp: ");
  Serial.print(temp, 1);
  Serial.print("C | Humi: ");
  Serial.print(humi, 0);
  Serial.print("% | Status: ");
  if (!sensorConnected) {
    Serial.print("No Sensor");
  } else {
    Serial.print(status);
  }
  Serial.println();
  delay(1000);
}