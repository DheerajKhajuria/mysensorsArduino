#include <SPI.h>
#include <MySensor.h>
#include <DHT.h>

#define CHILD_ID_HUM 0
#define CHILD_ID_TEMP 1

#define HUMIDITY_SENSOR_DIGITAL_PIN 3

unsigned long SLEEP_TIME = 105000; // Sleep time between reports (in milliseconds)

int BATTERY_SENSE_PIN = A0;  // select the input pin for the battery sense point
int oldValue=-1;
MySensor gw;
DHT dht;
float lastTemp;
float lastHum;
boolean metric = false;
int oldBatteryPcnt = 0;
int battLoop =0;

MyMessage msgHum(CHILD_ID_HUM, V_HUM);
MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);

boolean pinTriggered=0;//waitTime is number of seconds to hol in while loop
//long lastDebounceTime = 0;  // the last time the output pin was toggled
//long debounceDelay = 50;    // the debounce time; increase if the output flickers

void send_with_delay(MyMessage &msg, bool ack=false) 
{
  delay(1000);
  gw.send(msg,ack);
}

void setup()
{
  // use the 1.1 V internal reference
  //analogReference(EXTERNAL);
  gw.begin(NULL,16);
  dht.setup(HUMIDITY_SENSOR_DIGITAL_PIN);
  // Send the Sketch Version Information to the Gateway
  gw.sendSketchInfo("Humidity w/ Batt", "1.0");
  // Register all sensors to gw (they will be created as child devices)
  gw.present(CHILD_ID_HUM, S_HUM);
  gw.present(CHILD_ID_TEMP, S_TEMP);

  metric = gw.getConfig().isMetric;
  check_batt();
}

void loop()
{
  if (pinTriggered)
  {
    //Serial.println(millis()-lastDebounceTime);
    // Read digital motion value
    boolean value = digitalRead(DIGITAL_INPUT_SENSOR);
    // If the switch changed, due to noise or pressing
    if (value != oldValue) {
        //lastDebounceTime = millis();
        // Send in the new value
        gw.send(msg.set(value==HIGH ? "1" : "0"));
        battLoop++;
        oldValue = value;
        Serial.print("Mot: ");
        Serial.println(value);
    }
    pinTriggered=0;
  }
  else
  {
    delay(dht.getMinimumSamplingPeriod());
    float temperature = dht.getTemperature();
    if (isnan(temperature))
    {
      Serial.println("Failed reading temperature from DHT");
    }
    else if (temperature != lastTemp)
    {
      lastTemp = temperature;
      if (!metric)
      {
        temperature = dht.toFahrenheit(temperature);
      }
      send_with_delay(msgTemp.set(temperature, 1));
      battLoop++;
      Serial.print("T: ");
      Serial.println(temperature);
    }
    float humidity = dht.getHumidity();
    if (isnan(humidity))
    {
      Serial.println("Failed reading humidity from DHT");
    }
    else if (humidity != lastHum)
    {
      lastHum = humidity;
      send_with_delay(msgHum.set(humidity, 1));
      battLoop++;
      Serial.print("H: ");
      Serial.println(humidity);
    }
    if (battLoop > 10)
    {
      check_batt();
      battLoop=0;
    }
  }
  // Sleep until interrupt comes in on motion sensor. Send update every two minute.
   pinTriggered = gw.sleep(INTERRUPT, CHANGE, SLEEP_TIME);
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

  int batteryPcnt = sensorValue / 10 ;

  Serial.print("Battery Voltage: ");

  Serial.print(batteryV);

  Serial.println(" V");

  Serial.print("Battery percent: ");

  Serial.print(batteryPcnt);

  Serial.println(" %");



  if (oldBatteryPcnt != batteryPcnt) {

    // Power up radio after sleep

    gw.sendBatteryLevel(batteryPcnt);

    oldBatteryPcnt = batteryPcnt;

  }

}
