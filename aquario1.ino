// Bibliotecas
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "RTClib.h"
#include <Servo.h>

// Inicializações
RTC_DS1307 rtc;
Servo foodServo;

// Pin inititalization
int heaterRelayPin = 10;
int servoPin = 11;
int temperatureSensorPin = 12;
int temperatureLedPin = LED_BUILTIN;

// Temperature sensor
OneWire oneWire(temperatureSensorPin);
DallasTemperature tempSensor(&oneWire);
DeviceAddress temperatureSensorAddress;
int temperatureSensorErrors = 0;

// ** Variables **
bool bSerialAvailable = false; // Connected to a device

// Heater
int minTemperature = 24; // Min Temperature
int maxTemperature = 30; // Max Temperature
float temperature = 0;
bool bCheckTemperature = true;
bool bKeepHeaterOn = false;
int temperatureLedState = LOW;
int temperatureLedInterval = 1000;

// Time
unsigned long ulPreviousLedMillis = 0;
char timeBuffer[12];

// Feeder
int feedAngle = 180;
int feedInterval = 800;
int feedSchedule[] = {7, 13, 20}; // Feed Schedule
int feedTimes = 3; // Feed Times
int feedMinute = 0;
bool bFishFed = false;
int fedBoolAdjustMinute = feedMinute + 1;

void setup()
{
  Serial.begin(9600);
  if (!rtc.begin())
  {
    Serial.println("Relógio não inicializado");
    hardwareError();
    while (1)
      ;
  }
  else
  {
    pinMode(heaterRelayPin, OUTPUT);
    pinMode(temperatureLedPin, OUTPUT);

    foodServo.attach(servoPin);
    foodServo.write(0);
    foodServo.detach();

    tempSensor.begin();

    // while(Serial.available() == 0 && millis() <= 8000);

    if (bSerialAvailable)
    {
      Serial.println("Ajustando relógio");
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }

    DateTime currentTime = rtc.now();
    if(bSerialAvailable) Serial.println("Relógio inicializado");

    readTemperature();
    printTimeAndTemperature(currentTime);
  }
}

void loop()
{
  printTemperature();

  DateTime currentTime = rtc.now();

  checkTemperature(currentTime);
  checkFeedSchedule(currentTime);
}

void blinkLed(int temperatureLedInterval)
{
  unsigned long currentMillis = millis();
  if (currentMillis - ulPreviousLedMillis >= temperatureLedInterval)
  {
    ulPreviousLedMillis = currentMillis;
    if (temperatureLedState == LOW)
    {
      temperatureLedState = HIGH;
    }
    else
    {
      temperatureLedState = LOW;
    }
    digitalWrite(temperatureLedPin, temperatureLedState);
  }
}

void checkFeedSchedule(DateTime currentTime) {
  if(bFishFed == false && currentTime.minute() == feedMinute) {
    for (int i = 0; i < feedTimes; i++) {
      if (currentTime.hour() == feedSchedule[i]) {
        feedFish();
        if (bSerialAvailable) Serial.println("Fish Fed!!!");
        bFishFed = true;
      }
    }
  }
  if (bFishFed == true && currentTime.minute() == fedBoolAdjustMinute) {
    bFishFed = false;
  }
}

void checkTemperature(DateTime currentTime) {
  if (bCheckTemperature && (currentTime.second() == 0 || currentTime.second() == 30))
  {
    readTemperature();
    // setHeaterRelay();
    printTimeAndTemperature(currentTime);
    bCheckTemperature = false;
  }
  if (bCheckTemperature == false && (currentTime.second() == 1 || currentTime.second() == 31))
  {
    bCheckTemperature = true;
  }
}

void cycleServo(int angle, int step, int stepDelay, int betweenDelay)
{
  for (int a = 0; a <= angle; a += step)
  {
    foodServo.write(a);
    delay(stepDelay);
  }
  delay(betweenDelay);
  for (int a = angle; a >= 0; a -= step)
  {
    foodServo.write(a);
    delay(stepDelay);
  }
}

void feedFish()
{
  foodServo.attach(servoPin);
  // shake
  cycleServo(80, 40, 50, 100);
  cycleServo(80, 40, 50, 100);
  delay(100);
  // feed
  cycleServo(feedAngle, 10, 40, feedInterval);
  delay(500);
  foodServo.detach();
}

void hardwareError()
{
  temperatureLedState = LOW;
}

void printTemperature()
{
  // Erro de leitura
  if (temperatureSensorErrors)
  {
    hardwareError();
  }
  // Fria
  else if (temperature < minTemperature)
  {
    blinkLed(150);
  }
  // Ideal
  else if (temperature <= maxTemperature)
  {
    blinkLed(1000);
  }
  // Quente
  else
  {
    blinkLed(5000);
  }
}

void printTimeAndTemperature(DateTime currentTime)
{
  if (bSerialAvailable == false) return;
  sprintf(timeBuffer, "%02d:%02d:%02d ", currentTime.hour(), currentTime.minute(), currentTime.second());
  Serial.print("** ");
  Serial.print(timeBuffer);
  Serial.println();
  Serial.println(String(temperature) + " graus");
}

void readTemperature()
{
  if (!tempSensor.getAddress(temperatureSensorAddress, 0))
  {
    if (bSerialAvailable) Serial.println("Sensor de Temperatura falhou");
    temperatureSensorErrors += 1;
    return;
  }

  temperatureSensorErrors = 0;
  tempSensor.requestTemperatures();
  temperature = tempSensor.getTempC(temperatureSensorAddress);
}

void setHeaterRelay()
{
  if (temperatureSensorErrors >= 10)
  {
    if (bSerialAvailable) Serial.println("Relay Low");
    digitalWrite(heaterRelayPin, LOW);
    bKeepHeaterOn = false;
    return;
  }
  if (temperature <= minTemperature)
  {
    if (bSerialAvailable) Serial.println("Relay High");
    digitalWrite(heaterRelayPin, HIGH);
    bKeepHeaterOn = true;
  }
  else if (temperature > maxTemperature)
  {
    if (bSerialAvailable) Serial.println("Relay Low");
    digitalWrite(heaterRelayPin, LOW);
    bKeepHeaterOn = false;
  }
  else
  {
    if (bKeepHeaterOn)
    {
      if (bSerialAvailable) Serial.println("Relay High");
      digitalWrite(heaterRelayPin, HIGH);
    }
    else
    {
      if (bSerialAvailable) Serial.println("Relay Low");
      digitalWrite(heaterRelayPin, LOW);
    }
  }
}
