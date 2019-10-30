/*
  Halloween Remote Controller 02
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
byte mac[] = { 0xDC, 0x1A, 0x4D, 0x2B, 0xD5, 0x4D };

// IP address in case DHCP fails
IPAddress ip(192,168,0,22);

char serveraddr = "192.168.1.110";
byte myserver[] = { 192, 168, 0, 23 }; // zoomkat web page server IP address


// Ethernet comms
EthernetServer server(80);
EthernetClient ethclient;

// Create aREST instance
aREST rest = aREST();

// Variables to be exposed to the API
int pirsidwalkright;
int pirsidewalkleft;

// Declare functions to be exposed to the API
int pirdisable(String command);
int triggerterry(String command);
int triggerskull(String command);

/////////////////
// PIN MAPPING //
/////////////////

// MOSSFETS
int MOSSFET_1 = 9; // Yard Light CH1
int MOSSFET_2 = 6; // Yard Light CH2
int MOSSFET_3 = 5; // Yard Light CH3
int MOSSFET_4 = 3; // Yard Light CH4
int MOSSFET_5 = 2; // Yard Light CH5

// RELAYS
int RELAY_1 = A0; // Skull
int RELAY_2 = A1; // Spare
int RELAY_3 = A2; // Spare
int RELAY_4 = A3; // Spare

// PIRS
int PIR_1 = A4; //PIR Left Sidewalk
int PIR_2 = A5; //PIR Right Sidewalk

// DFPlayer (MP3 for scary terry)
int DF_RX = 4; // RX For MP3 Control
int DF_TX = 7; // TX For MP3 Control
int DF_BUSY = 8; // Lets us know if it's still playing a track

// DFPlayer Init
SoftwareSerial mySoftwareSerial(DF_RX, DF_TX);
DFRobotDFPlayerMini myDFPlayer;

// Variables

unsigned long currentMillis = 0;  
unsigned long previousLedMillis = 0;

unsigned long spookylightingdelay = 0;
unsigned long terrydelay = 0;
bool spookylighting = false;
bool skullactive = false;
bool terryactive = false;
bool piractive = true;

// Lightning Variables
unsigned long timeuntillightning = 0;
unsigned long flashduration = 0;
unsigned long timeuntilnextflash = 0;
int numflashes = 0;
byte lightningState = LOW;

// Barrel Variables
unsigned long skulldelay = 0;
unsigned long activeduration = 0;
unsigned long timeuntilnextactive = 0;
int numactive = 0;
byte skullState = LOW;


//void http_communicate_send(String ip, 
void setup(void)
{
  mySoftwareSerial.begin(9600);
  // Start Serial
  Serial.begin(115200);

  Serial.println(F("Initializing DFPlayer..."));

  if (!myDFPlayer.begin(mySoftwareSerial)) {  //Use softwareSerial to communicate with mp3.
    Serial.println(F("Unable to begin. Check connection and SD card, or reset the Arduino."));
    //while (true);
  }

  Serial.println(F("DFPlayer Mini online."));

  myDFPlayer.setTimeOut(500);                   // Set serial communictaion time out 500ms
  myDFPlayer.volume(30);                        // Set volume value (0~30).
  myDFPlayer.EQ(DFPLAYER_EQ_BASS);              // Set EQ to BASS (normal/pop/rock/jazz/classic/bass)
  myDFPlayer.outputDevice(DFPLAYER_DEVICE_SD);  // Set device we use SD as default
  myDFPlayer.enableDAC();                       // Enable On-chip DAC
  
  // Init variables and expose them to REST API
  pirsidwalkright = 0;
  pirsidewalkleft = 0;
  rest.variable("pirsidwalkright",&pirsidwalkright);
  rest.variable("pirsidewalkleft",&pirsidewalkleft);

  // Function to be exposed
  rest.function("pirdisable",pirdisable);
  rest.function("triggerterry",triggerterry);
  rest.function("triggerskull",triggerskull);

  // Give name & ID to the device (ID should be 6 characters long)
  rest.set_id("002");
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
  pinMode(RELAY_3, OUTPUT);
  pinMode(RELAY_4, OUTPUT);

  pinMode(PIR_1, INPUT);
  pinMode(PIR_2, INPUT);

  // Start the Ethernet connection and the server
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // no point in carrying on, so do nothing forevermore:
    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(mac, ip);
  }
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());

  // Start watchdog
  wdt_enable(WDTO_4S);

  // Let ethernet shield init:
  delay(1000);
}

void loop() {
  // if you get a connection, report back via serial:
  
  currentMillis = millis();
  // listen for incoming clients
  EthernetClient client = server.available();
  rest.handle(client);
  wdt_reset();

  byte pir1 = digitalRead(PIR_1);
  byte pir2 = digitalRead(PIR_2);
  //digitalWrite(RELAY_3, !pir1);
  //digitalWrite(RELAY_4, !pir2);
  ambientlighting();
  skulllogic(pir1);
  terrylogic(pir2);
  
  // check for serial input
  if (Serial.available() > 0) //if something in serial buffer
  {
    byte inChar; // sets inChar as a byte
    inChar = Serial.read(); //gets byte from buffer
    if(inChar == 'e') // checks to see byte is an e
    {
      sendGET(); // call sendGET function below when byte is an e
    }
  } 
}
void sendGET() //client function to send/receive GET request data.
{

Serial.print("test");
}

void ambientlighting() {
  //Serial.print("ambient");
  int flashCount = random (15, 45);        // Min. and max. number of flashes each loop
  int flashBrightnessMin =  25;           // LED flash min. brightness (0-255)
  int flashBrightnessMax =  200;          // LED flash max. brightness (0-255)

  int flashDurationMin = 1;               // Min. duration of each seperate flash
  int flashDurationMax = 255;              // Max. duration of each seperate flash

  int nextFlashDelayMin = 1;              // Min, delay between each flash and the next
  int nextFlashDelayMax = 600;            // Max, delay between each flash and the next

  int lightningdelaymin = 1000;
  int lightningdelaymax = 5000;

  if(spookylighting)
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
          int randombright = random(flashBrightnessMin, flashBrightnessMax);
          analogWrite(MOSSFET_1, randombright);
          analogWrite(MOSSFET_2, randombright);
          analogWrite(MOSSFET_3, randombright);
          analogWrite(MOSSFET_4, randombright);
          analogWrite(MOSSFET_5, randombright);
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
          analogWrite(MOSSFET_1, 25);
          analogWrite(MOSSFET_2, 25);
          analogWrite(MOSSFET_3, 25);
          analogWrite(MOSSFET_4, 25);
          analogWrite(MOSSFET_5, 25);
          lightningState = LOW;
          numflashes--;
          if(numflashes > 0)
          {
            timeuntilnextflash = random(nextFlashDelayMin, nextFlashDelayMax) + currentMillis;
          }
          else
          {
            spookylighting = false;
            spookylightingdelay = random(6000, 15000) + currentMillis;
          }
        }
      }
      
    }
  }
  else
  {
    analogWrite(MOSSFET_1, 255);
    analogWrite(MOSSFET_2, 255);
    analogWrite(MOSSFET_3, 255);
    analogWrite(MOSSFET_4, 255);
    analogWrite(MOSSFET_5, 255);
    if(currentMillis >= spookylightingdelay)
    {
          spookylighting = true;
    }
  }
  
}

void skulllogic(byte pir) {
  if(skullactive)
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
      if(skullState == LOW)
      {
          //Serial.print("Lightning low");
        if(currentMillis >= timeuntilnextactive)
        {
            //Serial.print("Lightning ON");
          digitalWrite(RELAY_1, HIGH);
          skullState = HIGH;

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
          skullState = LOW;
          numactive--;
          if(numactive > 0)
          {
            timeuntilnextactive = random(nextActivateDelayMin, nextActivateDelayMax) + currentMillis;
          }
          else
          {
            skulldelay = 25000 + currentMillis;
            skullactive = false;
          }
        }
      }
      
    }
  }
  else
  {
    if(currentMillis >= skulldelay)
    {
      if(piractive)
      {
        if(pir == HIGH)
        {
          skullactive = true;
        }
      }
    }
  }
}


void terrylogic(byte pir) {
  if(terryactive)
  {
    delay(1500);
    // If no file is currently being played
    if(digitalRead(DF_BUSY) == HIGH)
    {
     deactivateterry();
    }
  }
  else 
  {
    if(currentMillis >= terrydelay)
    {
      if(piractive)
      {
        if(pir == HIGH)
        {
          activateterry();
        }
      }
    }
  }
}

void activateterry()
{
  terryactive = true;
  http_terrylight(1);
  myDFPlayer.playMp3Folder(1);
}

void deactivateterry()
{
  terryactive = false;
  http_terrylight(0);
  myDFPlayer.pause();
  terrydelay = random(8000, 10000) + currentMillis;
}


void http_terrylight(int value) {
  if (ethclient.connect(myserver, 80)) {  //starts client connection, checks for connection
    Serial.println("connected");
    if(value) {
      ethclient.println("POST /triggerterrylight?state=1 HTTP/1.0"); //download text
    } else {
      ethclient.println("POST /triggerterrylight?state=0 HTTP/1.0"); //download text
    }
    
    ethclient.println(); //end of get request
  }
  else {
    Serial.println("connection failed"); //error message if no client connect
    Serial.println();
  }

  while(ethclient.connected() && !ethclient.available()) delay(1); //waits for data
  while (ethclient.connected() || ethclient.available()) { //connected or data available
    char c = ethclient.read(); //gets byte from ethernet buffer
    Serial.print(c); //prints byte to serial monitor
  }

  Serial.println();
  Serial.println("disconnecting.");
  Serial.println("==================");
  Serial.println();
  ethclient.stop(); //stop client
}
/*void lightning() {

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
          analogWrite(MOSSFET_1, random(flashBrightnessMin, flashBrightnessMax));
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
          analogWrite(MOSSFET_1, 50);
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
}*/
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
int triggerterry(String command) {

  // Get state from command
  int state = command.toInt();
  if(state)
  {
    activateterry();
  }
  else
  {
    deactivateterry();
  }
  return 1;

}
// Trigger Pneumatic Air Horn
int triggerskull(String command) {
  
  // Get state from command
  int state = command.toInt();

  if(state)
  {
    skullactive = true;
  }
  else
  {
    skullactive = false;
  }
  return 1;

}
