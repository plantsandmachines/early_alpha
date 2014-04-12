//__________________________________________
//
// 	           PLANTS & MACHINES
//            early alpha v-0.0.1
//
//              Martin Breuer
//
//                  GPLv2
//
//__________________________________________


#include <DHT.h>
#include <Arduino.h>
#include "get_airthsensor.ino"
#include "get_distance.ino"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <aJSON.h>

// switching high and low because of relay board config (relay is off on HIGH)
#define LIGHT_ON false
#define LIGHT_OFF true
#define PUMP_ON false
#define PUMP_OFF true

// actor state switches for ISR
volatile boolean lightStatus = LIGHT_ON;
volatile boolean pumpStatus  = PUMP_OFF;

// actors pins going to relays
const int light = A0;
const int pump = A1;
const int motor = A2;
const int fan  = A3;

// sensors pins
const int airthsensor = DHT11;
const int airthPin = 2;
const int waterPin = 3;
const int distPin = 4;

// define cycle times in seconds
const int lightOnFor = 43200;
const int lightOffFor = 43200;
const int floodEvery = 900;

// conter, use volatile to be able to increment and reset inside ISR
volatile int secLightOn   = 0;
volatile int secLightOff  = 0;
volatile int secLastFlood = 0;

// sensor readout
float distance = 0;
float airHumid = 0;
float airTemp = 0;
boolean water = 1;
DHT dht (airthPin, airthsensor);
aJsonStream serial_stream(&Serial);
 
void setup () 
{
  Serial.begin(9600);
  dht.begin();
 
  // apply INPUT and OUTPUT definition only for digital pins
  pinMode(airthPin, INPUT);
  pinMode(distPin, INPUT);
  pinMode(waterPin, INPUT);  
 
  // set ANALOG pins to act digital
  pinMode(pump, OUTPUT);
  pinMode(motor, OUTPUT);
  pinMode(fan, OUTPUT);
  pinMode(light, OUTPUT);

  // use internal pullup
  digitalWrite(waterPin, HIGH);
 
  // switch all actors off
  digitalWrite(pump, pumpStatus);
  digitalWrite(motor, LOW);
  digitalWrite(fan, LOW);
  digitalWrite(light, lightStatus);

  // initialize Timer1
  cli();         // disable global interrupts

  TCCR1A = 0;    // set entire TCCR1A register to 0
  TCCR1B = 0;    // same for TCCR1B
  
  OCR1A = 15624;          // set compare match register to desired timer count. 16 MHz with 1024 prescaler = 15624 counts/s 
  TCCR1B |= (1 << WGM12); // turn on CTC mode. clear timer on compare match
  
  // Set CS10 and CS12 bits for 1024 prescaler
  TCCR1B |= (1 << CS10);
  TCCR1B |= (1 << CS12);
  
  // enable timer compare interrupt:
  TIMSK1 |= (1 << OCIE1A);

  sei(); // enable global interrupts
}

  aJsonObject *createMessage(){
    aJsonObject *msg = aJson.createObject();
    aJson.addNumberToObject(msg, "temp", (float)airTemp);
    aJson.addNumberToObject(msg, "humid", (float)airHumid);
    aJson.addNumberToObject(msg, "water", (boolean)water);
    aJson.addNumberToObject(msg, "distance", (float)distance);
    aJson.addNumberToObject(msg, "lastFlood", secLastFlood/60);  
    return msg;    
  }
 
// gets called exacly once per second
ISR(TIMER1_COMPA_vect)
{
    if (lightStatus == LIGHT_ON)
    {
      secLightOn++;
      if (secLightOn == lightOnFor)
      {
        lightStatus = LIGHT_OFF;
        secLightOn = 0;
      }
    }

    else if (lightStatus == LIGHT_OFF)
    {
      secLightOff++;
      if (secLightOff == lightOffFor)
      {
        lightStatus = LIGHT_ON;
        secLightOff = 0;
      }
    }

   if (pumpStatus == PUMP_OFF)
   {
     secLastFlood++;
     if (secLastFlood == floodEvery)
     {
       pumpStatus = PUMP_ON;
       secLastFlood = 0;
     }
   } 
   
  else if (pumpStatus == PUMP_ON && water == true)
  {
    pumpStatus = PUMP_OFF;
  }
}

void rise ()
{
  distance = get_distance();
}


void cool ()
{
  airHumid = get_airHumid();

  if (airHumid > 45)
  {
    digitalWrite (fan, HIGH);
    // Serial.println ("FAN ON HUMIDITY HIGH");
  }

  else if (airHumid < 38)
  {
    digitalWrite (fan, LOW);
    // Serial.println ("FAN OFF HUMIDITY OKAY");
  }


  airTemp = get_airTemp();


  if (airTemp > 35)
  {
    digitalWrite (fan, HIGH);
    // Serial.println ("FAN ON TEMP HIGH");
  }
}

boolean get_water()
{
  return digitalRead(waterPin);
}

void loop () 
{

  rise();
  cool();
  water = get_water();

  digitalWrite (light, lightStatus);  
  digitalWrite (pump, pumpStatus);  
  
  aJsonObject *msg = createMessage();
  aJson.print(msg, &serial_stream);
  Serial.println();
  aJson.deleteItem(msg); 


}

