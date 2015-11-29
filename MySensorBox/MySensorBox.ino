#include <SPI.h>
#include <MySensor.h>
#include <DHT.h>
#include <IRLib.h>

#define CHILD_ID_HUM 0
#define CHILD_ID_TEMP 1
#define CHILD_ID_LIGHT 2
#define CHILD_ID_MOTION 3
#define CHILD_ID_IR 4

// Digital Pin for Sensors
#define HUMIDITY_SENSOR_DIGITAL_PIN 4
#define PIR_SENSOR_PIN 5
#define IR_RECV_PIN 8
#define BUZZER_PIN 6

//Analog pin
#define LIGHT_SENSOR_ANALOG_PIN A0
#define BATTERY_SENSE_PIN A1

#define EIGHTSECOND 8000

float lastTemp;
float lastHum;
boolean metric = false;
int oldValue;
int lastLightLevel;
//int oldBatteryPcnt;
//int battLoop;

unsigned long start;
IRsend irsend;
IRrecv irrecv(IR_RECV_PIN);
IRdecode decoder;
MySensor gw;
DHT dht;
//unsigned int Buffer[RAWBUF];
char * pch;
unsigned long incomingRelayStatus;
unsigned int irtypevalue;
unsigned int bitsvalue;


MyMessage msgHum(CHILD_ID_HUM, V_HUM);
MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);
MyMessage msgLight(CHILD_ID_LIGHT, V_LIGHT_LEVEL);
MyMessage msgMotion(CHILD_ID_MOTION, V_TRIPPED);
MyMessage msgIR(CHILD_ID_IR, V_VAR1);


void beep(unsigned char delayms){
  analogWrite(BUZZER_PIN, 16);      // Almost any value can be used except 0 and 255
  delay(delayms);                   // wait for a delayms ms
  analogWrite(BUZZER_PIN, 0);       // 0 turns it off
  delay(delayms);                   // wait for a delayms ms   
}  


void setup()
{
 
  gw.begin(incomingMessage, 16);
  dht.setup(HUMIDITY_SENSOR_DIGITAL_PIN);
  // Send the Sketch Version Information to the Gateway
  gw.sendSketchInfo("MySensorBox", "2.0");
  // Register all sensors to gw (they will be created as child devices)
  gw.present(CHILD_ID_HUM, S_HUM);
  gw.present(CHILD_ID_TEMP, S_TEMP);
  gw.present(CHILD_ID_LIGHT, S_LIGHT_LEVEL);
  gw.present(CHILD_ID_MOTION, S_MOTION);
  gw.present(CHILD_ID_IR, S_LIGHT);

  pinMode(PIR_SENSOR_PIN, INPUT);     // sets the motion sensor digital pin as input
  metric = gw.getConfig().isMetric;
  //check_volt();
  start = millis();
  irrecv.enableIRIn(); // Start the ir receiver
  pinMode(BUZZER_PIN, OUTPUT);
  beep(50);
  beep(50);
  beep(50);
  delay(1000);
}

void checklight()
{
  int lightLevel = (analogRead(LIGHT_SENSOR_ANALOG_PIN)) / 10.23;
  if (lightLevel != lastLightLevel) {
    Serial.print("L:");
    Serial.println(lightLevel);
    gw.send(msgLight.set(lightLevel));
    lastLightLevel = lightLevel;
  }
}
void checkDHT()
{
  delay(dht.getMinimumSamplingPeriod());
  float temperature = dht.getTemperature();
  if (isnan(temperature)) {
    Serial.println("Failed");
  } else if (temperature != lastTemp) {
    lastTemp = temperature;
    if (!metric) {
      temperature = dht.toFahrenheit(temperature);
    }
    gw.send(msgTemp.set(temperature, 1));
    Serial.print("T: ");
    Serial.println(temperature);
  }

  float humidity = dht.getHumidity();
  if (isnan(humidity)) {
    Serial.println("Failed");
  } else if (humidity != lastHum) {
    lastHum = humidity;
    gw.send(msgHum.set(humidity, 1));
    Serial.print("H: ");
    Serial.println(humidity);
  }
}

void checkMotion()
{
  // Read digital motion value
  boolean value = digitalRead(PIR_SENSOR_PIN);
  // If the switch changed, due to noise or pressing
  if (value != oldValue) {
    gw.send(msgMotion.set(value == HIGH ? "1" : "0"));
    if(value == HIGH ) beep(200);
    oldValue = value;
    Serial.print("Mot: ");
    Serial.println(value);
  }
}

void checkIRIn()
{
  if (irrecv.GetResults(&decoder)) {
    decoder.decode();
    decoder.DumpResults();

    if ( decoder.value != 0x00  && decoder.value != 0xffffffff )
    {
      char buffer[20];
      sprintf(buffer, "%d>%08lx>%d", decoder.decode_type, decoder.value, decoder.bits);
      gw.send(msgIR.set(buffer));
    }
    irrecv.resume();
  }
}
void loop()
{
  gw.process();
  unsigned long nowtime;
  checkMotion();
  nowtime = millis();
  
  if (nowtime - start > EIGHTSECOND*3 )
  { 
    //irsend.send(4, 0xc00010, 24);
    TIMSK2 = 0; // disable the  50us timer interrupt used by irlib.
    start = nowtime;
    checklight();
    checkDHT();
    irrecv.enableIRIn(); // Start the ir receiver
  }
    checkIRIn();
}

void incomingMessage(const MyMessage &message) { 
//   We only expect one type of message from controller. But we better check anyway.

    if (message.type==V_IR_SEND)
    {
       pch = strtok ((char*)message.getString(),":");
       incomingRelayStatus = strtoul(pch, NULL, 16);
       Serial.println(incomingRelayStatus);
       pch = strtok (NULL, ":");
       irtypevalue =  atoi(pch);
       Serial.println(irtypevalue);
       pch = strtok (NULL, ":");
       bitsvalue =  atoi(pch);
       Serial.println(bitsvalue);
      irsend.send(irtypevalue, incomingRelayStatus ,bitsvalue);
// Start receiving ir again...
      irrecv.enableIRIn();
    }

}


