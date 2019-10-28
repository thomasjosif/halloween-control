/*
  Halloween Remote Controller 01
  Copyright (C) 2019 Thomas Dick <thomasjosif@outlook.com>

  Using aRest by Marco Schwartz <https://github.com/marcoschwartz/aREST/>
*/

// Libraries
#include <SPI.h>
#include <Ethernet.h>
#include <aREST.h>
#include <avr/wdt.h>

// Enter a MAC address for your controller below.
byte mac[] = { 0x8C, 0x25, 0xB4, 0xC9, 0x4D, 0x2B };

// IP address in case DHCP fails
IPAddress ip(192,168,0,21);

// Ethernet server
EthernetServer server(80);

// Create aREST instance
aREST rest = aREST();

// Variables to be exposed to the API
int pirdrivewaytop;
int pirdrivewaybottom;

// Declare functions to be exposed to the API
int pirdisable(String command);
int triggeralarmlight(String command);
int triggerhorn(String command);

/////////////////
// PIN MAPPING //
/////////////////

// MOSSFETS
int MOSSFET_1 = A0; // Roof Lightning Channel
int MOSSFET_2 = A1; // Yard Light CH1
int MOSSFET_3 = A2; // Yard Light CH2
int MOSSFET_4 = A3; // Yard Light CH3
int MOSSFET_5 = A4; // Yard Light CH4

// RELAYS
int RELAY_1 = 2; // Warning Light
int RELAY_2 = 3; // Electrical Box Red Light
int RELAY_3 = 4; // Air Horn
int RELAY_4 = 5; // Spare

// PIRS
int PIR_1 = 6; //PIR Top of Driveway
int PIR_2 = 7; //PIR Bottom of Driveway


// Variables

unsigned long currentMillis = 0;  
unsigned long previousLedMillis = 0;


// Lightning Variables
unsigned long timeuntillightning = 0;
unsigned long flashduration = 0;
unsigned long timeuntilnextflash = 0;
int numflashes = 0;
byte lightningState = LOW;


void setup(void)
{
  // Start Serial
  Serial.begin(9600);
  Serial.print("Beas ");
  // Init variables and expose them to REST API
  pirdrivewaytop = 0;
  pirdrivewaybottom = 0;
//  rest.variable("pirdrivewaytop",&pirdrivewaytop);
 // rest.variable("pirdrivewaybottom",&pirdrivewaybottom);

  // Function to be exposed
 // rest.function("pirdisable",pirdisable);
 // rest.function("triggeralarmlight",triggeralarmlight);
  //rest.function("triggerhorn",triggerhorn);

  // Give name & ID to the device (ID should be 6 characters long)
  //rest.set_id("001");
  //rest.set_name("remote_control");

  /////////////////
  // PIN CONFIG  //
  /////////////////
  pinMode(MOSSFET_1, OUTPUT);
  pinMode(MOSSFET_2, OUTPUT);
  pinMode(MOSSFET_3, OUTPUT);
  pinMode(MOSSFET_4, OUTPUT);
  pinMode(MOSSFET_5, OUTPUT);

  pinMode(RELAY_1, OUTPUT);
  pinMode(RELAY_2, OUTPUT);
  pinMode(RELAY_3, OUTPUT);
  pinMode(RELAY_4, OUTPUT);

  pinMode(PIR_1, INPUT);
  pinMode(PIR_2, INPUT);

  // Start the Ethernet connection and the server
  //if (Ethernet.begin(mac) == 0) {
  //  Serial.println("Failed to configure Ethernet using DHCP");
    // no point in carrying on, so do nothing forevermore:
    // try to congifure using IP address instead of DHCP:
   // Ethernet.begin(mac, ip);
  //}
  //server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());

  // Start watchdog
  //wdt_enable(WDTO_4S);
}

void loop() {

  currentMillis = millis();
  // listen for incoming clients
  //EthernetClient client = server.available();
  //rest.handle(client);
  //wdt_reset();

  byte pir1 = digitalRead(PIR_1);
  byte pir2 = digitalRead(PIR_2);

  lightning();
}

void lightning() {

  int flashCount = random (3, 15);        // Min. and max. number of flashes each loop
  int flashBrightnessMin =  10;           // LED flash min. brightness (0-255)
  int flashBrightnessMax =  255;          // LED flash max. brightness (0-255)

  int flashDurationMin = 1;               // Min. duration of each seperate flash
  int flashDurationMax = 50;              // Max. duration of each seperate flash

  int nextFlashDelayMin = 1;              // Min, delay between each flash and the next
  int nextFlashDelayMax = 150;            // Max, delay between each flash and the next

  int lightningdelaymin = 1000;
  int lightningdelaymax = 5000;
  
  if(currentMillis >= timeuntillightning)
  {
    Serial.print("Execute lig");
    if(numflashes <= 0)
    {
      Serial.print("numflash ");
      numflashes = flashCount;
    }
    else
    {
      if(lightningState == LOW)
      {
          Serial.print("Lightning low");
        if(currentMillis >= timeuntilnextflash)
        {
            Serial.print("Lightning ON");
          analogWrite(MOSSFET_1, random(flashBrightnessMin, flashBrightnessMax));
          lightningState = HIGH;

          flashduration = random(flashDurationMin, flashDurationMax) + currentMillis;
        }
      }
      else
      {
          Serial.print("Lightning HIGH");
        if(currentMillis >= flashduration)
        {
            Serial.print("Lightning OFF");
          analogWrite(MOSSFET_1, 0);
          lightningState = LOW;
          numflashes--;
          if(numflashes > 0)
          {
            timeuntilnextflash = random(nextFlashDelayMin, nextFlashDelayMax) + currentMillis;
          }
          else
          {
            timeuntillightning = random(lightningdelaymin, lightningdelaymax) + currentMillis;
          }
        }
      }
      
    }
  }
}
// Disable PIR's for 10s
int pirdisable(String command) {

  // Get state from command
  //int state = command.toInt();

  //digitalWrite(6,state);
  //return 1;

}
// Trigger Electrical box alarm light
int triggeralarmlight(String command) {

  // Get state from command
  //int state = command.toInt();

  //digitalWrite(6,state);
  //return 1;

}
// Trigger Pneumatic Air Horn
int triggerhorn(String command) {

  // Get state from command
  //int state = command.toInt();

  //digitalWrite(6,state);
  //return 1;

}
