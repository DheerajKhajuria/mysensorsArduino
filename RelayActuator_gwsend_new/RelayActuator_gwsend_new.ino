// Example sketch showing how to control physical relays. 
// This example will remember relay state even after power failure.

// Enable debug prints
#define MY_DEBUG
#define MY_RADIO_RF24

// Enable repeater functionality for this node
#define MY_REPEATER_FEATURE

#include <MySensors.h>
#include <SPI.h>

#define RELAY_1  3  // Arduino Digital I/O pin number for first relay (second on pin+1 etc)
#define NUMBER_OF_RELAYS 2 // Total number of attached relays
#define RELAY_ON  0 // GPIO value to write to turn on attached relay
#define RELAY_OFF 1 // GPIO value to write to turn off attached relay
#define CHILD_ID NUMBER_OF_RELAYS+1

void presentation()
{
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo("Relay", "1.0");
  
  for (int sensor=1, pin=RELAY_1; sensor<=NUMBER_OF_RELAYS;sensor++, pin++)
  {
    // Register all sensors to gw (they will be created as child devices)
    present(sensor, S_LIGHT);
  }
 }
 
void setup()  
{   

}

void before()
{
  // Fetch relay status
  for (int sensor=1, pin=RELAY_1; sensor<=NUMBER_OF_RELAYS;sensor++, pin++) {
    // Then set relay pins in output mode
    pinMode(pin, OUTPUT);   
    // Set relay to last known state (using eeprom storage) 
    digitalWrite(pin,loadState(sensor)?RELAY_ON:RELAY_OFF);
  }
}

void loop() 
{

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
