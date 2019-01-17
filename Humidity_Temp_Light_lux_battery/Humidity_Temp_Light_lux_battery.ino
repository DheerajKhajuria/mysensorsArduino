

// Enable debug prints to serial monitor
#define MY_DEBUG

// Enable and select radio type attached
#define MY_RADIO_RF24

#include <SPI.h> 
#include <MySensors.h>  
#include <DHT.h> 
#include <BH1750.h> 

#include <avr/power.h>

#define CHILD_ID_HUM 0
#define CHILD_ID_TEMP 1
#define CHILD_ID_LIGHT 2
#define CHILD_ID_LUX 3

#define HUMIDITY_SENSOR_DIGITAL_PIN 4
#define BATTERY_SENSE_PIN  A0  // select the input pin for the battery sense point
#define LIGHT_SENSOR_ANALOG_PIN A1

unsigned long SLEEP_TIME = 180000; // Sleep time between reads (in milliseconds)

// Sleep time between sensor updates (in milliseconds)
// Must be >1000ms for DHT22 and >2000ms for DHT11
static const uint64_t UPDATE_INTERVAL = 60000;

// Force sending an update of the temperature after n sensor reads, so a controller showing the
// timestamp of the last update doesn't show something like 3 hours in the unlikely case, that
// the value didn't change since;
// i.e. the sensor would force sending an update every UPDATE_INTERVAL*FORCE_UPDATE_N_READS [ms]
static const uint8_t FORCE_UPDATE_N_READS = 10;

// Set this offset if the sensor has a permanent small offset to the real temperatures.
// In Celsius degrees (as measured by the device)
#define SENSOR_TEMP_OFFSET 0

DHT dht;
BH1750 lightSensor;
float lastTemp;
float lastHum;
uint16_t lastLux;
boolean metric = true; 
uint8_t nNoUpdatesTemp;
uint8_t nNoUpdatesHum;
MyMessage msgHum(CHILD_ID_HUM, V_HUM);
MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);
MyMessage msgLight(CHILD_ID_LIGHT, V_LIGHT_LEVEL);
MyMessage msgLux(CHILD_ID_LUX, V_LEVEL);
int oldBatteryPcnt = 0;
int battLoop =0;
int lightLevel =0;
int lastLightLevel =0;

void setup()  
{   
  dht.setup(HUMIDITY_SENSOR_DIGITAL_PIN); 
  lightSensor.begin();
  check_batt();

  if (UPDATE_INTERVAL <= dht.getMinimumSamplingPeriod()) {
   Serial.println("Warning: UPDATE_INTERVAL is smaller than supported by the sensor!");
  }
  // Sleep for the time of the minimum sampling period to give the sensor time to power up
  // (otherwise, timeout errors might occure for the first reading)
  sleep(dht.getMinimumSamplingPeriod());
}

void presentation()
{
  sendSketchInfo("Temp Humidity Light", "1.0");
  // Register all sensors to gw (they will be created as child devices)
  present(CHILD_ID_HUM, S_HUM);
  present(CHILD_ID_TEMP, S_TEMP);
  present(CHILD_ID_LIGHT, S_LIGHT_LEVEL);
  present(CHILD_ID_LUX, V_LEVEL);
  metric = getControllerConfig().isMetric;
 
}


void loop()
{
  calculate_temperature();
  calculate_humidity();
  calculate_lux();
  calculate_light();

  calculate_battery();
  sleep(SLEEP_TIME); //sleep a bit
}

void calculate_light()
{
  lightLevel = (analogRead(LIGHT_SENSOR_ANALOG_PIN)) / 10.23;
  Serial.print("lightLevel :");
  Serial.println(lightLevel);

  if (lightLevel != lastLightLevel) {
    send(msgLight.set(lightLevel));
    lastLightLevel = lightLevel;
  }
}
void calculate_lux()
{
  uint16_t lux = lightSensor.readLightLevel();// Get Lux value
  Serial.print("Lux :");
  Serial.println(lux);

  if (lux != lastLux) {
    send(msgLux.set(lux));
    lastLux = lux;
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
  // Get humidity from DHT library
  float humidity = dht.getHumidity();
  if (isnan(humidity)) {
    Serial.println("Failed reading humidity from DHT");
  } else if (humidity != lastHum || nNoUpdatesHum == FORCE_UPDATE_N_READS) {
    // Only send humidity if it changed since the last measurement or if we didn't send an update for n times
    lastHum = humidity;
    // Reset no updates counter
    nNoUpdatesHum = 0;
    send(msgHum.set(humidity, 1));

    #ifdef MY_DEBUG
    Serial.print("H: ");
    Serial.println(humidity);
    #endif
  } else {
    // Increase no update counter if the humidity stayed the same
    nNoUpdatesHum++;
  }
}

void calculate_temperature()
{
  // Force reading sensor, so it works also after sleep()
  dht.readSensor(true);
  // Get temperature from DHT library
  float temperature = dht.getTemperature();
  if (isnan(temperature)) {
    Serial.println("Failed reading temperature from DHT!");
  } else if (temperature != lastTemp || nNoUpdatesTemp == FORCE_UPDATE_N_READS) {
    // Only send temperature if it changed since the last measurement or if we didn't send an update for n times
    lastTemp = temperature;

    // apply the offset before converting to something different than Celsius degrees
    temperature += SENSOR_TEMP_OFFSET;

    if (!metric) {
      temperature = dht.toFahrenheit(temperature);
    }
    // Reset no updates counter
    nNoUpdatesTemp = 0;
    send(msgTemp.set(temperature, 1));

    #ifdef MY_DEBUG
    Serial.print("T: ");
    Serial.println(temperature);
    #endif
  } else {
    // Increase no update counter if the temperature stayed the same
    nNoUpdatesTemp++;
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
    sendBatteryLevel(batteryPcnt);
    oldBatteryPcnt = batteryPcnt;
  }

}


//
//
