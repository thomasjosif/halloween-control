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
byte myserver[] = { 192, 168, 1, 110 }; // zoomkat web page server IP address


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
int MOSSFET_1 = A0; // Yard Light CH1
int MOSSFET_2 = A1; // Yard Light CH2
int MOSSFET_3 = A2; // Yard Light CH3
int MOSSFET_4 = A3; // Yard Light CH4
int MOSSFET_5 = A4; // Yard Light CH5

// RELAYS
int RELAY_1 = 2; // Skull
int RELAY_2 = 3; // Spare
int RELAY_3 = 4; // Spare
int RELAY_4 = 5; // Spare

// PIRS
int PIR_1 = A5; //PIR Left Sidewalk
int PIR_2 = 9; //PIR Right Sidewalk

// DFPlayer (MP3 for scary terry)
int DF_RX = 6; // RX For MP3 Control
int DF_TX = 7; // TX For MP3 Control
int DF_BUSY = 8; // Lets us know if it's still playing a track

// DFPlayer Init
SoftwareSerial mySoftwareSerial(DF_RX, DF_TX);
DFRobotDFPlayerMini myDFPlayer;

// Variables

unsigned long currentMillis = 0;  
unsigned long previousLedMillis = 0;


// Lightning Variables
unsigned long timeuntillightning = 0;
unsigned long flashduration = 0;
unsigned long timeuntilnextflash = 0;
int numflashes = 0;
byte lightningState = LOW;

//void http_communicate_send(String ip, 
void setup(void)
{
  // Start Serial
  Serial.begin(9600);
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

  terry();
  lightning();
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
  if (ethclient.connect(myserver, 80)) {  //starts client connection, checks for connection
    Serial.println("connected");
    ethclient.println("GET / HTTP/1.0"); //download text
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

void terry() {
  int delaybetweentalking = 7000;

  // If terry is currently talking
  if(digitalRead(DF_BUSY) == LOW)
  {
    //http_communicate_send(REMOTE_03, "triggerterrylight", 1);
  }
  else
  {
    //http_communicate_send(REMOTE_03, "triggerterrylight", 0);
  }
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
}
// Disable PIR's for 10s
int pirdisable(String command) {

  // Get state from command
  //int state = command.toInt();

  //digitalWrite(6,state);
  //return 1;

}
// Trigger Electrical box alarm light
int triggerterry(String command) {

  // Get state from command
  //int state = command.toInt();

  //digitalWrite(6,state);
  //return 1;

}
// Trigger Pneumatic Air Horn
int triggerskull(String command) {

  // Get state from command
  //int state = command.toInt();

  //digitalWrite(6,state);
  //return 1;

}
