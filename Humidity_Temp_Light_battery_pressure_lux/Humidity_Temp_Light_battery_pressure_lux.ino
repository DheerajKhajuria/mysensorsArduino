#include <SPI.h>
#include <MySensor.h>
#include <DHT.h>
#include <BH1750.h>
#include <Wire.h>
#include <avr/power.h>
#include <Adafruit_BMP085.h>

#define CHILD_ID_HUM 0
#define CHILD_ID_TEMP 1
#define CHILD_ID_LIGHT 2
#define CHILD_ID_LUX 3
#define CHILD_ID_BARO 4
#define CHILD_ID_BARO_TEMP 5

MyMessage msgHum(CHILD_ID_HUM, V_HUM);
MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);
MyMessage msgLight(CHILD_ID_LIGHT, V_LIGHT_LEVEL);
MyMessage msgLux(CHILD_ID_LUX, V_LEVEL);
MyMessage msgBaroTemp(CHILD_ID_BARO_TEMP, V_TEMP);
MyMessage msgBaropressure(CHILD_ID_BARO, V_PRESSURE);
MyMessage msgBaroforecast(CHILD_ID_BARO, V_FORECAST);

#define HUMIDITY_SENSOR_DIGITAL_PIN 4
#define BATTERY_SENSE_PIN  A0  // select the input pin for the battery sense point
#define LIGHT_SENSOR_ANALOG_PIN A1
// this CONVERSION_FACTOR is used to convert from Pa to kPa in forecast algorithm
// get kPa/h be dividing hPa by 10
#define CONVERSION_FACTOR (1.0/10.0)

const float ALTITUDE = 235; //  Gurgaon altitude
unsigned long SLEEP_TIME = 60000; // Sleep time between reads (in milliseconds)

MySensor gw;
DHT dht;
BH1750 lightSensor;
Adafruit_BMP085 bmp = Adafruit_BMP085();      // Digital Pressure Sensor

float lastTemp      = -1;
float lastHum       = -1;
float lastPressure  = -1;
float lastBaroTemp  = -1;
int lastForecast    = -1;
int lastLightLevel  = 0;

boolean metric = true;

const int LAST_SAMPLES_COUNT = 5;
float lastPressureSamples[LAST_SAMPLES_COUNT];
int minuteCount = 0;
bool firstRound = true;
// average value is used in forecast algorithm.
float pressureAvg;
// average after 2 hours is used as reference value for the next iteration.
float pressureAvg2;
float dP_dt;
int oldBatteryPcnt = 0;
int battLoop = 0;
int lightLevel = 0;
uint16_t lastlux;

const char *weather[] = { "stable", "sunny", "cloudy", "unstable", "thunderstorm", "unknown" };
enum FORECAST
{
  STABLE = 0,     // "Stable Weather Pattern"
  SUNNY = 1,      // "Slowly rising Good Weather", "Clear/Sunny "
  CLOUDY = 2,     // "Slowly falling L-Pressure ", "Cloudy/Rain "
  UNSTABLE = 3,   // "Quickly rising H-Press",     "Not Stable"
  THUNDERSTORM = 4, // "Quickly falling L-Press",    "Thunderstorm"
  UNKNOWN = 5     // "Unknown (More Time needed)
};

void setup()
{
  gw.begin(NULL, 15);

  dht.setup(HUMIDITY_SENSOR_DIGITAL_PIN);
  // Send the Sketch Version Information to the Gateway
  gw.sendSketchInfo("Temp Hum Light Lux Pres", "1.0");

  // Register all sensors to gw (they will be created as child devices)
  gw.present(CHILD_ID_HUM, S_HUM);
  gw.present(CHILD_ID_TEMP, S_TEMP);
  gw.present(CHILD_ID_LIGHT, S_LIGHT_LEVEL);
  gw.present(CHILD_ID_LUX, V_LEVEL);
  gw.present(CHILD_ID_BARO, S_BARO);
  gw.present(CHILD_ID_BARO_TEMP, S_TEMP);

  metric = gw.getConfig().isMetric;

  lightSensor.begin();

  if (!bmp.begin())
  {
    Serial.println("Could not find a valid BMP180 sensor, check wiring!");
    while (1) {}
  }
  check_batt();
}

void loop()
{
  calculate_pressure_temp();
  calculate_pressure_forecast();
  calculate_temperature();
  calculate_humidity();
  calculate_lux();
  calculate_light();

  calculate_battery();
  gw.sleep(SLEEP_TIME); //sleep a bit
}

void calculate_light()
{
  lightLevel = (analogRead(LIGHT_SENSOR_ANALOG_PIN)) / 10.23;
  Serial.print("lightLevel :");
  Serial.println(lightLevel);

  if (lightLevel != lastLightLevel) {
    gw.send(msgLight.set(lightLevel));
    lastLightLevel = lightLevel;
  }
}
void calculate_lux()
{
  uint16_t lux = lightSensor.readLightLevel();// Get Lux value
  Serial.print("Lux :");
  Serial.println(lux);

  if (lux != lastlux) {
    gw.send(msgLux.set(lux));
    lastlux = lux;
  }
}

void calculate_battery()
{
  battLoop++;
  if (battLoop > 10)
  {
    check_batt();
    battLoop = 0;
  }
}

void calculate_humidity()
{
  float humidity = dht.getHumidity();
  if (isnan(humidity)) {
    Serial.println("Failed reading humidity from DHT");
  } else if (humidity != lastHum) {

    lastHum = humidity;
    gw.send(msgHum.set(humidity, 1));
    Serial.print("H: ");
    Serial.println(humidity);
  }
}

void calculate_temperature()
{
  delay(dht.getMinimumSamplingPeriod());
  float temperature = dht.getTemperature();
  if (isnan(temperature)) {
    Serial.println("Failed reading temperature from DHT");
  } else if (temperature != lastTemp) {
    lastTemp = temperature;
    if (!metric) {
      temperature = dht.toFahrenheit(temperature);
    }
    gw.send(msgTemp.set(temperature, 1));
    Serial.print("T: ");
    Serial.println(temperature);
  }
}

void calculate_pressure_temp()
{
  float temperature = bmp.readTemperature();

  if (!metric)
  {
    // Convert to fahrenheit
    temperature = temperature * 9.0 / 5.0 + 32.0;
  }
  Serial.print("Temperature = ");
  Serial.print(temperature);
  Serial.println(metric ? " *C" : " *F");

  if (temperature != lastBaroTemp)
  {
    gw.send(msgBaroTemp.set(temperature, 1));
    lastTemp = temperature;
  }
}

void calculate_pressure_forecast()
{
  float pressure = bmp.readSealevelPressure(ALTITUDE) / 100.0;

  int forecast = sample(pressure);

  Serial.print("Pressure = ");
  Serial.print(pressure);
  Serial.println(" hPa");
  Serial.print("Forecast = ");
  Serial.println(weather[forecast]);
  if (pressure != lastPressure)
  {
    gw.send(msgBaropressure.set(pressure, 0));
    lastPressure = pressure;
  }
  if (forecast != lastForecast)
  {
    gw.send(msgBaroforecast.set(weather[forecast]));
    lastForecast = forecast;
  }
}

void check_batt()
{
  // get the battery Voltage
  int sensorValue = analogRead(BATTERY_SENSE_PIN);
  Serial.println(sensorValue);

  // 1M, 470K divider across battery and using internal ADC ref of 1.1V
  // Sense point is bypassed with 0.1 uF cap to reduce noise at that point
  // ((1e6+470e3)/470e3)*1.1 = Vmax = 3.44 Volts
  // 3.44/1023 = Volts per bit = 0.003363075
  // vout = (sensorvalue x 3.3) / 1023
  //vin = vout /  (R2/(R1+R2));

  float batteryV  = ( sensorValue * 0.003225806 ) / 0.3; // divide R2/(R1+R2)
  int batteryPcnt = (sensorValue * 2.5) / 10 ;
  if ( batteryPcnt > 100 )
    batteryPcnt = 100;

  Serial.print("Battery Voltage: ");
  Serial.print(batteryV);
  Serial.println(" V");
  Serial.print("Battery percent: ");
  Serial.print(batteryPcnt);
  Serial.println(" %");

  if (oldBatteryPcnt != batteryPcnt)
  {
    // Power up radio after sleep
    gw.sendBatteryLevel(batteryPcnt);
    oldBatteryPcnt = batteryPcnt;
  }

}

// Algorithm found here
// http://www.freescale.com/files/sensors/doc/app_note/AN3914.pdf
// Pressure in hPa -->  forecast done by calculating kPa/h
int sample(float pressure)
{
  // Calculate the average of the last n minutes.
  int index = minuteCount % LAST_SAMPLES_COUNT;
  lastPressureSamples[index] = pressure;

  minuteCount++;
  if (minuteCount > 185)
  {
    minuteCount = 6;
  }

  if (minuteCount == 5)
  {
    pressureAvg = getLastPressureSamplesAverage();
  }
  else if (minuteCount == 35)
  {
    float lastPressureAvg = getLastPressureSamplesAverage();
    float change = (lastPressureAvg - pressureAvg) * CONVERSION_FACTOR;
    if (firstRound) // first time initial 3 hour
    {
      dP_dt = change * 2; // note this is for t = 0.5hour
    }
    else
    {
      dP_dt = change / 1.5; // divide by 1.5 as this is the difference in time from 0 value.
    }
  }
  else if (minuteCount == 65)
  {
    float lastPressureAvg = getLastPressureSamplesAverage();
    float change = (lastPressureAvg - pressureAvg) * CONVERSION_FACTOR;
    if (firstRound) //first time initial 3 hour
    {
      dP_dt = change; //note this is for t = 1 hour
    }
    else
    {
      dP_dt = change / 2; //divide by 2 as this is the difference in time from 0 value
    }
  }
  else if (minuteCount == 95)
  {
    float lastPressureAvg = getLastPressureSamplesAverage();
    float change = (lastPressureAvg - pressureAvg) * CONVERSION_FACTOR;
    if (firstRound) // first time initial 3 hour
    {
      dP_dt = change / 1.5; // note this is for t = 1.5 hour
    }
    else
    {
      dP_dt = change / 2.5; // divide by 2.5 as this is the difference in time from 0 value
    }
  }
  else if (minuteCount == 125)
  {
    float lastPressureAvg = getLastPressureSamplesAverage();
    pressureAvg2 = lastPressureAvg; // store for later use.
    float change = (lastPressureAvg - pressureAvg) * CONVERSION_FACTOR;
    if (firstRound) // first time initial 3 hour
    {
      dP_dt = change / 2; // note this is for t = 2 hour
    }
    else
    {
      dP_dt = change / 3; // divide by 3 as this is the difference in time from 0 value
    }
  }
  else if (minuteCount == 155)
  {
    float lastPressureAvg = getLastPressureSamplesAverage();
    float change = (lastPressureAvg - pressureAvg) * CONVERSION_FACTOR;
    if (firstRound) // first time initial 3 hour
    {
      dP_dt = change / 2.5; // note this is for t = 2.5 hour
    }
    else
    {
      dP_dt = change / 3.5; // divide by 3.5 as this is the difference in time from 0 value
    }
  }
  else if (minuteCount == 185)
  {
    float lastPressureAvg = getLastPressureSamplesAverage();
    float change = (lastPressureAvg - pressureAvg) * CONVERSION_FACTOR;
    if (firstRound) // first time initial 3 hour
    {
      dP_dt = change / 3; // note this is for t = 3 hour
    }
    else
    {
      dP_dt = change / 4; // divide by 4 as this is the difference in time from 0 value
    }
    pressureAvg = pressureAvg2; // Equating the pressure at 0 to the pressure at 2 hour after 3 hours have past.
    firstRound = false; // flag to let you know that this is on the past 3 hour mark. Initialized to 0 outside main loop.
  }

  int forecast = UNKNOWN;
  if (minuteCount < 35 && firstRound) //if time is less than 35 min on the first 3 hour interval.
  {
    forecast = UNKNOWN;
  }
  else if (dP_dt < (-0.25))
  {
    forecast = THUNDERSTORM;
  }
  else if (dP_dt > 0.25)
  {
    forecast = UNSTABLE;
  }
  else if ((dP_dt > (-0.25)) && (dP_dt < (-0.05)))
  {
    forecast = CLOUDY;
  }
  else if ((dP_dt > 0.05) && (dP_dt < 0.25))
  {
    forecast = SUNNY;
  }
  else if ((dP_dt > (-0.05)) && (dP_dt < 0.05))
  {
    forecast = STABLE;
  }
  else
  {
    forecast = UNKNOWN;
  }

 // uncomment when debugging
//  Serial.print(F("Forecast at minute "));
//  Serial.print(minuteCount);
//  Serial.print(F(" dP/dt = "));
//  Serial.print(dP_dt);
//  Serial.print(F("kPa/h --> "));
//  Serial.println(weather[forecast]);

  return forecast;
}

float getLastPressureSamplesAverage()
{
  float lastPressureSamplesAverage = 0;
  for (int i = 0; i < LAST_SAMPLES_COUNT; i++)
  {
    lastPressureSamplesAverage += lastPressureSamples[i];
  }
  lastPressureSamplesAverage /= LAST_SAMPLES_COUNT;

  return lastPressureSamplesAverage;
}
//
