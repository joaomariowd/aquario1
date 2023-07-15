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
int temperatureMonitorLedPin = LED_BUILTIN;

// Temperature sensor
OneWire oneWire(temperatureSensorPin);
DallasTemperature tempSensor(&oneWire);
DeviceAddress temperatureSensorAddress;

// ** Variables **
// Heater
int temperatureMonitorLedState = LOW;
float temperature = 0;
int tempError = 0;
bool keepHeaterOn = false;
int minTemperature = 25;
int maxTemperature = 27.5;

// Time
unsigned long previousMillis = 0;
int interval = 1000;
bool adjustTime = false; // adjust time
bool time = true;
char timeBuffer[12];

// Feeder
int feedMorningHour = 7;
int feedMorningMinute = 0;
int feedNightHour = 19;
int feedNightMinute = 0;
int feedAngle = 180;
int feedInterval = 300;
bool feed = true;

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
    pinMode(temperatureMonitorLedPin, OUTPUT);

    foodServo.attach(servoPin);
    foodServo.write(5);
    foodServo.detach();

    tempSensor.begin();

    if (adjustTime == true)
    {
      Serial.println("Ajustando relógio");
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }

    DateTime currentTime = rtc.now();
    Serial.println("Relógio inicializado");

    setTemperature();
    setKeepHeaterOn();
    printTime(currentTime);
  }
}

void loop()
{
  // Hora e temperatura a cada 30s
  DateTime currentTime = rtc.now();

  if ((currentTime.second() == 0 || currentTime.second() == 30) && time)
  {
    setTemperature();
    heaterRelay();
    printTime(currentTime);
    time = false;
  }
  if (currentTime.second() == 1 || currentTime.second() == 31)
  {
    time = true;
  }
  temperatureMonitor();

  // Alimentação
  if ((
          (currentTime.hour() == feedMorningHour && currentTime.minute() == feedMorningMinute && currentTime.second() == 5) || (currentTime.hour() == feedNightHour && currentTime.minute() == feedNightMinute && currentTime.second() == 5)) &&
      feed)
  {
    feed = false;
    feedFish();
  }
  if ((
          (currentTime.hour() == feedMorningHour && currentTime.minute() == feedMorningMinute && currentTime.second() == 6) || (currentTime.hour() == feedNightHour && currentTime.minute() == feedNightMinute && currentTime.second() == 6)))
  {
    feed = true;
  }
}

void printTime(DateTime currentTime)
{
  sprintf(timeBuffer, "%02d:%02d:%02d ", currentTime.hour(), currentTime.minute(), currentTime.second());
  Serial.print("** ");
  Serial.print(timeBuffer);
  Serial.println();
  Serial.println(String(temperature) + " graus");
}

void setTemperature()
{
  tempSensor.requestTemperatures();
  if (!tempSensor.getAddress(temperatureSensorAddress, 0))
  {
    Serial.println("Sensor de Temperatura falhou");
    tempError += 1;
    return;
  }

  tempError = 0;
  temperature = tempSensor.getTempC(temperatureSensorAddress);
}

void setKeepHeaterOn()
{
  if (tempError > 0)
    keepHeaterOn = false;
  else if (temperature < minTemperature)
    keepHeaterOn = true;
  else
    keepHeaterOn = false;
}

void heaterRelay()
{
  if (tempError >= 10)
  {
    Serial.println("Relay Low");
    digitalWrite(heaterRelayPin, LOW);
    return;
  }
  if (temperature <= minTemperature)
  {
    Serial.println("Relay High");
    digitalWrite(heaterRelayPin, HIGH);
    keepHeaterOn = true;
  }
  else if (temperature > maxTemperature)
  {
    Serial.println("Relay Low");
    digitalWrite(heaterRelayPin, LOW);
    keepHeaterOn = false;
  }
  else
  {
    if (keepHeaterOn)
    {
      Serial.println("Relay High");
      digitalWrite(heaterRelayPin, HIGH);
    }
    else
    {
      Serial.println("Relay Low");
      digitalWrite(heaterRelayPin, LOW);
    }
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

void temperatureMonitor()
{
  // Erro de leitura
  if (tempError)
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

void blinkLed(int interval)
{
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval)
  {
    previousMillis = currentMillis;
    if (temperatureMonitorLedState == LOW)
    {
      temperatureMonitorLedState = HIGH;
    }
    else
    {
      temperatureMonitorLedState = LOW;
    }
    digitalWrite(temperatureMonitorLedPin, temperatureMonitorLedState);
  }
}

void hardwareError()
{
  temperatureMonitorLedState = LOW;
}
