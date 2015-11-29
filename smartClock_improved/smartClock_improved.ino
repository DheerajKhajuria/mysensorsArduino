#include <MaxMatrix.h>
#include <avr/pgmspace.h>
#include <Time.h>
#include <SPI.h>
#include <MySensor.h>
#include <DHT.h>
#include <QueueList.h>


const unsigned char CH[] PROGMEM = {
  3, 8, B0000000, B0000000, B0000000, B0000000, B0000000, // space
  1, 8, B1011111, B0000000, B0000000, B0000000, B0000000, // !
  3, 8, B0000011, B0000000, B0000011, B0000000, B0000000, // "
  5, 8, B0010100, B0111110, B0010100, B0111110, B0010100, // #
  4, 8, B0100100, B1101010, B0101011, B0010010, B0000000, // $
  5, 8, B1100011, B0010011, B0001000, B1100100, B1100011, // %
  5, 8, B0110110, B1001001, B1010110, B0100000, B1010000, // &
  1, 8, B0000011, B0000000, B0000000, B0000000, B0000000, // '
  3, 8, B0011100, B0100010, B1000001, B0000000, B0000000, // (
  3, 8, B1000001, B0100010, B0011100, B0000000, B0000000, // )
  5, 8, B0101000, B0011000, B0001110, B0011000, B0101000, // *
  5, 8, B0001000, B0001000, B0111110, B0001000, B0001000, // +
  2, 8, B10110000, B1110000, B0000000, B0000000, B0000000, // ,
  4, 8, B0001000, B0001000, B0001000, B0001000, B0000000, // -
  2, 8, B1100000, B1100000, B0000000, B0000000, B0000000, // .
  4, 8, B1100000, B0011000, B0000110, B0000001, B0000000, // /
  4, 8, B0111110, B1000001, B1000001, B0111110, B0000000, // 0
  4, 8, B1000010, B1111111, B1000000, B0000000, B0000000, // 1
  4, 8, B1100010, B1010001, B1001001, B1000110, B0000000, // 2
  4, 8, B0100010, B1000001, B1001001, B0110110, B0000000, // 3
  4, 8, B0011000, B0010100, B0010010, B1111111, B0000000, // 4
  4, 8, B0100111, B1000101, B1000101, B0111001, B0000000, // 5
  4, 8, B0111110, B1001001, B1001001, B0110000, B0000000, // 6
  4, 8, B1100001, B0010001, B0001001, B0000111, B0000000, // 7
  4, 8, B0110110, B1001001, B1001001, B0110110, B0000000, // 8
  4, 8, B0000110, B1001001, B1001001, B0111110, B0000000, // 9
  2, 8, B01010000, B0000000, B0000000, B0000000, B0000000, // :
  2, 8, B10000000, B01010000, B0000000, B0000000, B0000000, // ;
  3, 8, B0010000, B0101000, B1000100, B0000000, B0000000, // <
  3, 8, B0010100, B0010100, B0010100, B0000000, B0000000, // =
  3, 8, B1000100, B0101000, B0010000, B0000000, B0000000, // >
  4, 8, B0000010, B1011001, B0001001, B0000110, B0000000, // ?
  5, 8, B0111110, B1001001, B1010101, B1011101, B0001110, // @
  4, 8, B1111110, B0010001, B0010001, B1111110, B0000000, // A
  4, 8, B1111111, B1001001, B1001001, B0110110, B0000000, // B
  4, 8, B0111110, B1000001, B1000001, B0100010, B0000000, // C
  4, 8, B1111111, B1000001, B1000001, B0111110, B0000000, // D
  4, 8, B1111111, B1001001, B1001001, B1000001, B0000000, // E
  4, 8, B1111111, B0001001, B0001001, B0000001, B0000000, // F
  4, 8, B0111110, B1000001, B1001001, B1111010, B0000000, // G
  4, 8, B1111111, B0001000, B0001000, B1111111, B0000000, // H
  3, 8, B1000001, B1111111, B1000001, B0000000, B0000000, // I
  4, 8, B0110000, B1000000, B1000001, B0111111, B0000000, // J
  4, 8, B1111111, B0001000, B0010100, B1100011, B0000000, // K
  4, 8, B1111111, B1000000, B1000000, B1000000, B0000000, // L
  5, 8, B1111111, B0000010, B0001100, B0000010, B1111111, // M
  5, 8, B1111111, B0000100, B0001000, B0010000, B1111111, // N
  4, 8, B0111110, B1000001, B1000001, B0111110, B0000000, // O
  4, 8, B1111111, B0001001, B0001001, B0000110, B0000000, // P
  4, 8, B0111110, B1000001, B1000001, B10111110, B0000000, // Q
  4, 8, B1111111, B0001001, B0001001, B1110110, B0000000, // R
  4, 8, B1000110, B1001001, B1001001, B0110010, B0000000, // S
  5, 8, B0000001, B0000001, B1111111, B0000001, B0000001, // T
  4, 8, B0111111, B1000000, B1000000, B0111111, B0000000, // U
  5, 8, B0001111, B0110000, B1000000, B0110000, B0001111, // V
  5, 8, B0111111, B1000000, B0111000, B1000000, B0111111, // W
  5, 8, B1100011, B0010100, B0001000, B0010100, B1100011, // X
  5, 8, B0000111, B0001000, B1110000, B0001000, B0000111, // Y
  4, 8, B1100001, B1010001, B1001001, B1000111, B0000000, // Z
  2, 8, B1111111, B1000001, B0000000, B0000000, B0000000, // [
  4, 8, B0000001, B0000110, B0011000, B1100000, B0000000, // backslash
  2, 8, B1000001, B1111111, B0000000, B0000000, B0000000, // ]
  3, 8, B0000010, B0000001, B0000010, B0000000, B0000000, // hat
  4, 8, B1000000, B1000000, B1000000, B1000000, B0000000, // _
  2, 8, B0000001, B0000010, B0000000, B0000000, B0000000, // `
  4, 8, B0100000, B1010100, B1010100, B1111000, B0000000, // a
  4, 8, B1111111, B1000100, B1000100, B0111000, B0000000, // b
  4, 8, B0111000, B1000100, B1000100, B0101000, B0000000, // c
  4, 8, B0111000, B1000100, B1000100, B1111111, B0000000, // d
  4, 8, B0111000, B1010100, B1010100, B0011000, B0000000, // e
  3, 8, B0000100, B1111110, B0000101, B0000000, B0000000, // f
  4, 8, B10011000, B10100100, B10100100, B01111000, B0000000, // g
  4, 8, B1111111, B0000100, B0000100, B1111000, B0000000, // h
  3, 8, B1000100, B1111101, B1000000, B0000000, B0000000, // i
  4, 8, B1000000, B10000000, B10000100, B1111101, B0000000, // j
  4, 8, B1111111, B0010000, B0101000, B1000100, B0000000, // k
  3, 8, B1000001, B1111111, B1000000, B0000000, B0000000, // l
  5, 8, B1111100, B0000100, B1111100, B0000100, B1111000, // m
  4, 8, B1111100, B0000100, B0000100, B1111000, B0000000, // n
  4, 8, B0111000, B1000100, B1000100, B0111000, B0000000, // o
  4, 8, B11111100, B0100100, B0100100, B0011000, B0000000, // p
  4, 8, B0011000, B0100100, B0100100, B11111100, B0000000, // q
  4, 8, B1111100, B0001000, B0000100, B0000100, B0000000, // r
  4, 8, B1001000, B1010100, B1010100, B0100100, B0000000, // s
  3, 8, B0000100, B0111111, B1000100, B0000000, B0000000, // t
  4, 8, B0111100, B1000000, B1000000, B1111100, B0000000, // u
  5, 8, B0011100, B0100000, B1000000, B0100000, B0011100, // v
  5, 8, B0111100, B1000000, B0111100, B1000000, B0111100, // w
  5, 8, B1000100, B0101000, B0010000, B0101000, B1000100, // x
  4, 8, B10011100, B10100000, B10100000, B1111100, B0000000, // y
  3, 8, B1100100, B1010100, B1001100, B0000000, B0000000, // z
  3, 8, B0001000, B0110110, B1000001, B0000000, B0000000, // {
  1, 8, B1111111, B0000000, B0000000, B0000000, B0000000, // |
  3, 8, B1000001, B0110110, B0001000, B0000000, B0000000, // }
  4, 8, B0001000, B0000100, B0001000, B0000100, B0000000, // ~
};

#define CHILD_ID_HUM 0
#define CHILD_ID_TEMP 1
#define CHILD_ID_MOTION 2
#define CHILD_ID_DISPLAY 3

#define PIR_SENSOR_PIN 3
#define LOAD_PIN 4
#define DATA_PIN  5
#define BUZZER_PIN 6
#define HUMIDITY_SENSOR_DIGITAL_PIN 7
#define CLOCK_PIN  8
#define MAX_QUEUE 20

boolean timeReceived = false, showtime = false;
unsigned long lastUpdate = 0, lastRequest = 0 , lastBlink = 0 , lastDHT = 0, lastDisplay = 0;
int maxInUse = 4;    //change this variable to set how many MAX7219's you'll use
byte buffer[10];
boolean onoff = true;
char timevalue[10];
char AmPm[3];
float lastTemp;
float lastHum;
boolean metric = false;
int numValue = 0;
char *value;
int oldValue;
boolean beepOnOff = false, MelodyOnOff = false;
int length = 15; // the number of notes
char notes[] = "ccggaagffeeddc "; // a space represents a rest
int beats[] = { 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 4 };
char names[] = { 'c', 'd', 'e', 'f', 'g', 'a', 'b', 'C' };
int tones[] = { 1915, 1700, 1519, 1432, 1275, 1136, 1014, 956 };
int tempo = 300;

MySensor gw;
MyMessage msgHum(CHILD_ID_HUM, V_HUM);
MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);
MyMessage msgMotion(CHILD_ID_MOTION, V_TRIPPED);
MaxMatrix m(DATA_PIN, LOAD_PIN, CLOCK_PIN, maxInUse);
DHT dht;
QueueList <String> queue;

// This is called when a new time value was received
void receiveTime(unsigned long time) {
  // Ok, set incoming time
  setTime(time);
  Serial.println("Requested time received..");
  adjustTime(19800);  //UTC+5.30
  timeReceived = true;
  showTime();
}

void setup()
{
  gw.begin(incomingMessage, 20);
  m.init();
  Serial.print("Display Brightness  ");
  Serial.println(gw.loadState(CHILD_ID_DISPLAY));
  m.setIntensity(gw.loadState(CHILD_ID_DISPLAY) * 0.15);
  printStringWithShift("Con.Gw.", 10);

  // Send the sketch version information to the gateway and Controller
  gw.sendSketchInfo("MatrixClock", "1.0");
  dht.setup(HUMIDITY_SENSOR_DIGITAL_PIN);

  // Send the Sketch Version Information to the Gateway
  metric = gw.getConfig().isMetric;
  // Register all sensors to gw (they will be created as child devices)
  gw.present(CHILD_ID_HUM, S_HUM);
  gw.present(CHILD_ID_TEMP, S_TEMP);
  gw.present(CHILD_ID_MOTION, S_MOTION);

  pinMode(PIR_SENSOR_PIN, INPUT);     // sets the motion sensor digital pin as input
  pinMode(BUZZER_PIN, OUTPUT);

  // Send the sketch version information to the gateway and Controller
  // Request time from controller.
  gw.requestTime(receiveTime);
}

void inline checkMotion()
{
  // Read digital motion value
  boolean pirvalue = digitalRead(PIR_SENSOR_PIN);
  // If the switch changed, due to noise or pressing
  if (pirvalue != oldValue) {
    gw.send(msgMotion.set(pirvalue == HIGH ? "1" : "0"));
    oldValue = pirvalue;
    Serial.print("Mot: ");
    Serial.println(pirvalue);
  }
}

void inline beep(unsigned char delayms) {
  analogWrite(BUZZER_PIN, 16);      // Almost any value can be used except 0 and 255
  delay(delayms);                   // wait for a delayms ms
  analogWrite(BUZZER_PIN, 0);       // 0 turns it off
  delay(delayms);                   // wait for a delayms ms
}


void playTone(int tone, int duration) {
  for (long i = 0; i < duration * 1000L; i += tone * 2) {
    digitalWrite(BUZZER_PIN, HIGH);
    delayMicroseconds(tone);
    digitalWrite(BUZZER_PIN, LOW);
    delayMicroseconds(tone);
  }
}

void playNote(char note, int duration) {
  // play the tone corresponding to the note name
  for (int i = 0; i < 8; i++) {
    if (names[i] == note) {
      playTone(tones[i], duration);
    }
  }
}

void Melody() {
  for (int i = 0; i < length; i++) {
    if (notes[i] == ' ') {
      delay(beats[i] * tempo); // rest
    } else {
      playNote(notes[i], beats[i] * tempo);
    }

    // pause between notes
    delay(tempo / 2);
  }
}


void loop()
{
  checkMotion();
  unsigned long now = millis();
  gw.process();
  // If no time has been received yet, request it every 10 second from controller
  // When time has been received, request update 10 min
  if ((!timeReceived && now - lastRequest > 10000)
      || (timeReceived && now - lastRequest > 600000)) {
    // Request time from controller..
    Serial.println("Requesting time..");
    gw.requestTime(receiveTime);
    lastRequest = now;
  }

  if (timeReceived && now - lastUpdate > 59000)
  {
    showTime();
    lastUpdate = now;
  }

  // blink every second
  if (now - lastBlink > 1000) {
    if (showtime && timeReceived )
    {
      Blink();
    }
    lastBlink = now;
  }

  if ( now - lastDHT > 300000 )
  {
    checkDHT();
    lastDHT = now;
  }

  if ( beepOnOff == true ) beep(100);
  if ( MelodyOnOff == true ) Melody();

  if ( now - lastDisplay > 15000 && !queue.isEmpty ())
  {
    DisplayMsg();
    lastUpdate = now;
    lastDisplay = now;
  }

}

void inline DisplayMsg()
{
  String str = queue.pop ();
  m.clear();
  showtime = false;
  printStringWithShift( str.c_str(), 40 );
  delay(500);
  m.clear();
  printStringWithShift( str.c_str(), 40 );
  delay(500);
  showTime();
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

void inline Blink()
{
  onoff ^= 1;
  m.setDot(10, 3, onoff);
  m.setDot(10, 5, onoff);
}

void inline showTime()
{
  if (timeReceived)
  {
    m.clear();
    isAM() ? strcpy(AmPm, "am") : strcpy(AmPm, "pm");
    sprintf(timevalue, "%02d%02d%s", hourFormat12(), minute(), AmPm);
    printStringWithShift(timevalue, 3);
    showtime = true;
  }
}

void inline printCharWithShift(char c, int shift_speed)
{
  if (c < 32) return;
  c -= 32;
  memcpy_P(buffer, CH + 7 * c, 7);
  m.writeSprite(32, 0, buffer);
  m.setColumn(32 + buffer[0], 0);
  for (int i = 0; i < buffer[0] + 1; i++)
  {
    delay(shift_speed);
    m.shiftLeft(false, false);
  }
}

void inline printStringWithShift(const char* s, int shift_speed)
{
  while (*s != 0)
  {
    printCharWithShift(*s, shift_speed);
    s++;
  }
}

void incomingMessage(const MyMessage &message) {
  //   We only expect one type of message from controller. But we better check anyway.

  switch (message.type)
  {
    case V_VAR1:

      if (queue.count() < MAX_QUEUE)
      {
        queue.push (message.getString());
      }
      else
      {
        Serial.print("Error: Queue Full..Ignoring msg  ");
        Serial.println(message.getString());
      }

      break;
    case V_VAR2:


      numValue = message.getInt();
      if (numValue  != 0 && numValue < 100)
      {
        Serial.print("Incoming Msg: Brightness level  " );
        Serial.println(numValue);
        m.setIntensity(numValue * 0.15);
        // save the brightnessvalue to eeprom.
        gw.saveState(CHILD_ID_DISPLAY, numValue);
      }



      break;
    case V_VAR3: break;
    case V_VAR4: break;
    case V_VAR5: break;

  }
  if (message.type == V_IR_SEND)
  {
    value =  (char*)message.getString();
    numValue = atoi( value );

    if (numValue  != 0 && numValue < 100)
    {
      Serial.print("Incoming Msg: Brightness level  " );
      Serial.println(numValue);
      m.setIntensity(numValue * 0.15);
      // save the brightnessvalue to eeprom.
      gw.saveState(CHILD_ID_DISPLAY, numValue);
    }
    else if (numValue != 0)
    {
      beepOnOff = false; MelodyOnOff = false;
      switch (numValue)
      {
        case 101: beepOnOff = true; break;
        case 102: beepOnOff = false; break;
        case 103: MelodyOnOff = true; break;
        case 104: MelodyOnOff = false; break;
        default : beepOnOff = false; MelodyOnOff = false;
      }
    }
    else
    {
      if (queue.count() < MAX_QUEUE)
      {
        queue.push (message.getString());
      }
      else
      {
        Serial.print("Error: Queue Full..Ignoring msg  ");
        Serial.println(message.getString());
      }
    }
  }
}


