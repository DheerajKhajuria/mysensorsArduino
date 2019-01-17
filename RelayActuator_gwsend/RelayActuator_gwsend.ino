// Example sketch showing how to control physical relays. 
// This example will remember relay state even after power failure.

// Enable debug prints
#define MY_DEBUG

#define MY_RADIO_RF24

#include <MySensors.h>
#include <SPI.h>

#define RELAY_1  3  // Arduino Digital I/O pin number for first relay (second on pin+1 etc)
#define NUMBER_OF_RELAYS 2 // Total number of attached relays
#define RELAY_ON  0 // GPIO value to write to turn on attached relay
#define RELAY_OFF 1 // GPIO value to write to turn off attached relay
#define CHILD_ID NUMBER_OF_RELAYS+1

const int currentPin = 0;
const int effectivevoltage = 240; // 230v 
int WAIT_1 = 50 , WAIT_2 = 10000;

const unsigned long sampleTime = 100000UL;   // sample over 100ms, it is an exact number of cycles for both 50Hz and 60Hz mains
const unsigned long numSamples = 250UL;  // choose the number of samples to divide sampleTime exactly, but low enough for the ADC to keep up
const unsigned long sampleInterval = sampleTime/numSamples;  // the sampling interval, must be longer than then ADC conversion time
//const int adc_zero = 522;                                                     // relative digital zero of the arudino input from ACS712 (could make this a variable and auto-adjust it)
int adc_zero; //autoadjusted relative digital zero

int currWatt,prevWatt =0 ;
//float currKWh,preKWh = 0 ;
float current = 0;

MyMessage wattMsg(CHILD_ID,V_WATT);
MyMessage kwhMsg(CHILD_ID,V_KWH);


void presentation()
{
  for (int sensor=1, pin=RELAY_1; sensor<=NUMBER_OF_RELAYS;sensor++, pin++)
  {
    // Register all sensors to gw (they will be created as child devices)
    present(sensor, S_LIGHT);
  }
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo("Energy Meter", "1.0");
  // Register this device as power sensor
  present(CHILD_ID, S_POWER);
}


  
void setup()  
{   

  // Fetch relay status
  for (int sensor=1, pin=RELAY_1; sensor<=NUMBER_OF_RELAYS;sensor++, pin++) {
    // Then set relay pins in output mode
    pinMode(pin, OUTPUT);   
    // Set relay to last known state (using eeprom storage) 
    digitalWrite(pin,loadState(sensor)?RELAY_ON:RELAY_OFF);
  }
  
  adc_zero = determineVQ(currentPin); //Quiscent output voltage - the average voltage ACS712 shows with no load (0 A)
  delay(1000);  
}

void loop() 
{
  /*
  bool slept_enough = false;
  unsigned long start = millis();
  unsigned long now;
  // Let serial prints finish (debug, log etc)
  Serial.flush();
  current = 0 ;
  
  while (!slept_enough) {
         // Always process incoming messages whenever possible
         now = millis();
         if (now - start > WAIT_1) {
			slept_enough = true;
                        current = readCurrent(currentPin) ;
	     }
  }
  
  slept_enough = false;                               
  
  while (!slept_enough) {
         // Always process incoming messages whenever possible
         now = millis();
         if (now - start > WAIT_2) {
           if(current > 0.09 ) 
           {
            currWatt = current*effectivevoltage;
            if (currWatt != prevWatt )
              {
               Serial.print("ACS712@A2 :");Serial.print(current,4);Serial.println(" A");
               Serial.print("Wattage :");Serial.print(currWatt);Serial.println(" Watt");
               send(wattMsg.set(currWatt)); 
               prevWatt = currWatt;
              }
//            currKWh = currWatt * starttime / (1000 * 3600)
//            gw.send(kwhMsg.set(currKWh, 4));  // Send kwh value to gw 
          }
	     slept_enough = true;           
	 }
  }
  */
}

void receive(const MyMessage &message) {
  // We only expect one type of message from controller. But we better check anyway.
  if (message.type==V_LIGHT) {
     // Change relay state
     digitalWrite(message.sensor-1+RELAY_1, message.getBool()?RELAY_ON:RELAY_OFF);
     // Store state in eeprom
     saveState(message.sensor, message.getBool());
     // Write some debug info
     Serial.print("Incoming change for sensor:");
     Serial.print(message.sensor);
     Serial.print(", New status: ");
     Serial.println(message.getBool());
   } 
}

int determineVQ(int PIN) {
  Serial.print("estimating avg. quiscent voltage:");
  long VQ = 0;
  //read 5000 samples to stabilise value
  for (int i=0; i<5000; i++) {
    VQ += analogRead(PIN);
    delay(1);//depends on sampling (on filter capacitor), can be 1/80000 (80kHz) max.
  }
  VQ /= 5000;
  Serial.print(map(VQ, 0, 1023, 0, 5000));Serial.println(" mV");
  Serial.print(VQ);
  return int(VQ);
}

float readCurrent(int PIN)
{
  unsigned long currentAcc = 0;
  unsigned int count = 0;
  unsigned long prevMicros = micros() - sampleInterval ;
  while (count < numSamples)
  {
    if (micros() - prevMicros >= sampleInterval)
    {
      int adc_raw = analogRead(currentPin) - adc_zero;
      currentAcc += (unsigned long)(adc_raw * adc_raw);
      ++count;
      prevMicros += sampleInterval;
    }
  }
  float rms = sqrt((float)currentAcc/(float)numSamples) * (50 / 1024.0);
  return rms;
  //Serial.println(rms);
}
