/*
  Halloween Remote Controller 03
  Copyright (C) 2019 Thomas Dick <thomasjosif@outlook.com>

  Using aRest by Marco Schwartz <https://github.com/marcoschwartz/aREST/>
  Lightning logic inspired by: https://oneguyoneblog.com/2017/11/01/lightning-thunder-arduino-halloween-diy/
*/

// Libraries
#include <SPI.h>
#include <Ethernet.h>
#include <aREST.h>
#include <avr/wdt.h>
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>

// Enter a MAC address for your controller below.
byte mac[] = { 0x04, 0x63, 0xEC, 0x4F, 0x1A, 0xF8 };

// IP address in case DHCP fails
IPAddress ip(192,168,0,23);

// Ethernet comms
EthernetServer server(80);

// Create aREST instance
aREST rest = aREST();

// Variables to be exposed to the API
int pirbellbox;

// Declare functions to be exposed to the API
int pirdisable(String command);
int triggerbarrel(String command);
int triggerterrylight(String command);

/////////////////
// PIN MAPPING //
/////////////////

// MOSSFETS
int MOSSFET_1 = 9; // Terry CH1
int MOSSFET_2 = 6; // Barrel CH2
int MOSSFET_3 = 5; // Yard Light CH3
int MOSSFET_4 = 3; // Spare
int MOSSFET_5 = 0; // Spare

// RELAYS
int RELAY_1 = 2; // Barrel
int RELAY_2 = 4; // Spare


// PIRS
int PIR_1 = A5; //PIR Bell Box


// Variables

unsigned long currentMillis = 0;  
unsigned long previousLedMillis = 0;

bool barrelactive = false;
bool terryactive = false;
bool terryfadein = false;
bool piractive = true;
int terryfadelevel = 0;

// Barrel Variables
unsigned long barreldelay = 0;
unsigned long activeduration = 0;
unsigned long timeuntilnextactive = 0;
int numactive = 0;
byte barrelState = LOW;

// Lightning Variables
unsigned long timeuntillightning = 0;
unsigned long flashduration = 0;
unsigned long timeuntilnextflash = 0;
int numflashes = 0;
byte lightningState = LOW;

// Was going to make the above variables arrays, but since this is a rush job It'll just be like this for now. 
unsigned long timeuntillightning2 = 0;
unsigned long flashduration2 = 0;
unsigned long timeuntilnextflash2 = 0;
int numflashes2 = 0;
byte lightningState2 = LOW;

void setup(void)
{
  // Start Serial
  Serial.begin(9600);
  // Init variables and expose them to REST API
  pirbellbox = 0;
  rest.variable("pirbellbox",&pirbellbox);

  // Function to be exposed
  rest.function("pirdisable",pirdisable);
  rest.function("triggerbarrel",triggerbarrel);
  rest.function("triggerterrylight",triggerterrylight);

  // Give name & ID to the device (ID should be 6 characters long)
  rest.set_id("003");
  rest.set_name("remote_control");

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

  pinMode(PIR_1, INPUT);

  // Start the Ethernet connection and the server
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // no point in carrying on, so do nothing forevermore:
    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(mac, ip);
  }
  server.begin();
  Serial.print("Server is at ");
  Serial.println(Ethernet.localIP());

  // Start watchdog
  wdt_enable(WDTO_4S);

  // Let ethernet shield init:
  delay(1000);
}

void loop() {
  
  currentMillis = millis();
  // listen for incoming clients
  EthernetClient client = server.available();
  rest.handle(client);
  wdt_reset();

  byte pir1 = digitalRead(PIR_1);
  //digitalWrite(RELAY_2, digitalRead(PIR_1));
  barrellogic(pir1);
  
  terrylighting();
  barrellighting();
}


void terrylighting() {
  if(terryactive)
  {
      if(terryfadelevel < 255)
      {
        Serial.print("fadin");
        terryfadelevel++;
        analogWrite(MOSSFET_1, terryfadelevel);
      }
      else
      {
        analogWrite(MOSSFET_1, 255);
      }
  }
  else
  {
      if(terryfadelevel > 0)
      {
        terryfadelevel--;
        analogWrite(MOSSFET_1, terryfadelevel);
        Serial.print("fadeout");
      }
      else if(terryfadelevel <= 0)
      {
        terryfadelevel = 0;
        analogWrite(MOSSFET_1, 0);
      }
  }
  
}

void barrellighting() {

  int flashCount = random (3, 15);        // Min. and max. number of flashes each loop
  int flashBrightnessMin =  25;           // LED flash min. brightness (0-255)
  int flashBrightnessMax =  110;          // LED flash max. brightness (0-255)

  int flashDurationMin = 1;               // Min. duration of each seperate flash
  int flashDurationMax = 255;              // Max. duration of each seperate flash

  int nextFlashDelayMin = 1;              // Min, delay between each flash and the next
  int nextFlashDelayMax = 600;            // Max, delay between each flash and the next

  int lightningdelaymin = 1000;
  int lightningdelaymax = 5000;

  // We want spooky lighting when the prop is not active. When it's active we want FULL light.
  if(!barrelactive)
  {
    //Serial.print("Execute lig");
    if(numflashes <= 0)
    {
      //Serial.print("numflash ");
      numflashes = flashCount;
    }
    else
    {
      if(lightningState == LOW)
      {
          //Serial.print("Lightning low");
        if(currentMillis >= timeuntilnextflash)
        {
            //Serial.print("Lightning ON");
          analogWrite(MOSSFET_2, random(flashBrightnessMin, flashBrightnessMax));
          lightningState = HIGH;

          flashduration = random(flashDurationMin, flashDurationMax) + currentMillis;
        }
      }
      else
      {
          //Serial.print("Lightning HIGH");
        if(currentMillis >= flashduration)
        {
            //Serial.print("Lightning OFF");
          analogWrite(MOSSFET_2, 25);
          lightningState = LOW;
          numflashes--;
          if(numflashes > 0)
          {
            timeuntilnextflash = random(nextFlashDelayMin, nextFlashDelayMax) + currentMillis;
          }
        }
      }
      
    }

    // HACK REEEE
    //Serial.print("Execute lig");
    if(numflashes2 <= 0)
    {
      //Serial.print("numflash ");
      numflashes2 = random(3, 15);
    }
    else
    {
      if(lightningState2 == LOW)
      {
          //Serial.print("Lightning low");
        if(currentMillis >= timeuntilnextflash2)
        {
            //Serial.print("Lightning ON");
          analogWrite(MOSSFET_3, random(flashBrightnessMin, flashBrightnessMax));
          lightningState2 = HIGH;

          flashduration2 = random(flashDurationMin, flashDurationMax) + currentMillis;
        }
      }
      else
      {
          //Serial.print("Lightning HIGH");
        if(currentMillis >= flashduration2)
        {
            //Serial.print("Lightning OFF");
          analogWrite(MOSSFET_3, 25);
          lightningState2 = LOW;
          numflashes2--;
          if(numflashes2 > 0)
          {
            timeuntilnextflash2 = random(nextFlashDelayMin, nextFlashDelayMax) + currentMillis;
          }
        }
      }
      
    }
  }
  else
  {
    analogWrite(MOSSFET_2, 255);
  }
}

void barrellogic(byte pir) {
  if(barrelactive)
  {
    int activateCount = random (4, 10);    
  
    int activateDurationMin = 1;               
    int activateDurationMax = 750;           
  
    int nextActivateDelayMin = 1;              
    int nextActivateDelayMax = 500;

    if(numactive <= 0)
    {
      //Serial.print("numflash ");
      numactive = activateCount;
    }
    else
    {
      if(barrelState == LOW)
      {
          //Serial.print("Lightning low");
        if(currentMillis >= timeuntilnextactive)
        {
            //Serial.print("Lightning ON");
          digitalWrite(RELAY_1, HIGH);
          barrelState = HIGH;

          activeduration = random(activateDurationMin, activateDurationMax) + currentMillis;
        }
      }
      else
      {
          //Serial.print("Lightning HIGH");
        if(currentMillis >= activeduration)
        {
            //Serial.print("Lightning OFF");
          digitalWrite(RELAY_1, LOW);
          barrelState = LOW;
          numactive--;
          if(numactive > 0)
          {
            timeuntilnextactive = random(nextActivateDelayMin, nextActivateDelayMax) + currentMillis;
          }
          else
          {
            barreldelay = 25000 + currentMillis;
            barrelactive = false;
          }
        }
      }
      
    }
  }
  else
  {
    if(currentMillis >= barreldelay)
    {
      if(piractive)
      {
        if(pir == HIGH)
        {
          barrelactive = true;
        }
      }
    }
  }
}
// Disable PIR's for 10s
int pirdisable(String command) {

  // Get state from command
  int state = command.toInt();

  if(state)
  {
    piractive = true;
  }
  else
  {
    piractive = false;
  }
  return 1;

}
// Trigger Electrical box alarm light
int triggerbarrel(String command) {

  // Get state from command
  int state = command.toInt();

  if(state)
  {
    barrelactive = true;
  }
  else
  {
    barrelactive = false;
  }
  return 1;

}
// Trigger Pneumatic Air Horn
int triggerterrylight(String command) {

  // Get state from command
  int state = command.toInt();

  if(state)
  {
    terryactive = true;
  }
  else
  {
    terryactive = false;
  }
  return 1;

}
