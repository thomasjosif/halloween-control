/*
  Halloween Remote Controller 01
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

// Enter a MAC adCdress for your controller below.
byte mac[] = { 0x8C, 0x25, 0xB4, 0xC9, 0x4D, 0x2B };

// IP address in case DHCP fails
IPAddress ip(192,168,0,21);

char serveraddr = "192.168.1.110";
byte myserver[] = { 192, 168, 1, 20 }; // zoomkat web page server IP address


// Ethernet comms
EthernetServer server(80);
EthernetClient ethclient;

// Create aREST instance
aREST rest = aREST();

// Variables to be exposed to the API
int pirdrivewaytop;
int pirdrivewaybottom;

// Declare functions to be exposed to the API
int pirdisable(String command);
int triggerelectrical(String command);
int triggerhorn(String command);

/////////////////
// PIN MAPPING //
/////////////////

// MOSSFETS
int MOSSFET_1 = 9; // Yard Light CH1
int MOSSFET_2 = 6; // Yard Light CH2
int MOSSFET_3 = 5; // Yard Light CH3
int MOSSFET_4 = 3; // Yard Light CH4

// RELAYS
int RELAY_1 = A0; // Electrical Box Warning Light
int RELAY_2 = A1; // Electrical Box Inside Light
int RELAY_3 = A2; // Air Horn
int RELAY_4 = A3; // Spare

// PIRS
int PIR_1 = A4; //PIR Top Driveway
int PIR_2 = A5; //PIR Bottom Driveway

// Variables

unsigned long currentMillis = 0;  
unsigned long previousLedMillis = 0;

unsigned long spookylightingdelay = 0;
unsigned long terrydelay = 0;
bool spookylighting = false;
bool hornactive = false;
bool piractive = true;
bool electricalactive = false;

// Lightning Variables
unsigned long timeuntillightning = 0;
unsigned long flashduration = 0;
unsigned long timeuntilnextflash = 0;
int numflashes = 0;
byte lightningState = LOW;

// Lightning Variables
unsigned long timeuntillightning1 = 0;
unsigned long flashduration1 = 0;
unsigned long timeuntilnextflash1 = 0;
int numflashes1 = 0;
byte lightningState1 = LOW;

// Barrel Variables
unsigned long electricaldelay = 0;
unsigned long redlightoffdelay = 0;
unsigned long firecrackerdelay = 0;
unsigned long lastlightdelay = 0;
byte alarmlightstate = LOW;

unsigned long horntime = 0;
unsigned long horndelay = 0;



//void http_communicate_send(String ip, 
void setup(void)
{
  // Start Serial
  Serial.begin(115200);

  // Init variables and expose them to REST API
  pirdrivewaytop = 0;
  pirdrivewaybottom = 0;
  rest.variable("pirdrivewaytop",&pirdrivewaytop);
  rest.variable("pirdrivewaybottom",&pirdrivewaybottom);

  // Function to be exposed
  rest.function("pirdisable",pirdisable);
  rest.function("triggerelectrical",triggerelectrical);
  rest.function("triggerhorn",triggerhorn);

  // Give name & ID to the device (ID should be 6 characters long)
  rest.set_id("001");
  rest.set_name("remote_control");

  /////////////////
  // PIN CONFIG  //
  /////////////////
  pinMode(MOSSFET_1, OUTPUT);
  pinMode(MOSSFET_2, OUTPUT);
  pinMode(MOSSFET_3, OUTPUT);
  pinMode(MOSSFET_4, OUTPUT);

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
  electricallogic(pir1);
  hornlogic(pir2);
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
  analogWrite(MOSSFET_1, 255);

}

void ambientlighting() {

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
          analogWrite(MOSSFET_2, randombright);
          analogWrite(MOSSFET_3, randombright);
          analogWrite(MOSSFET_4, randombright);
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
          analogWrite(MOSSFET_3, 25);
          analogWrite(MOSSFET_4, 25);
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
    analogWrite(MOSSFET_2, 255);
    analogWrite(MOSSFET_3, 255);
    analogWrite(MOSSFET_4, 255);
    if(currentMillis >= spookylightingdelay)
    {
          spookylighting = true;
    }
  }
  
}



void electricallogic(byte pir) {
  if(electricalactive)
  {
    if(currentMillis >= firecrackerdelay)
    {
      electricalactive = false;
      http_electrical_alarm(0);
      http_electrical_firecracker(1);
      digitalWrite(RELAY_2, HIGH);
      redlightoffdelay = currentMillis + 2000;
      electricaldelay = currentMillis + 8000;
    }

    if(currentMillis >= lastlightdelay)
    {
        if(alarmlightstate == HIGH)
        {
          lastlightdelay = currentMillis + 750;
          digitalWrite(RELAY_1, LOW);    
          alarmlightstate = LOW;
        }
        else
        {
          lastlightdelay = currentMillis + 750;
          digitalWrite(RELAY_1, HIGH);
          alarmlightstate = HIGH;
        }
    }
  }
  else
  {
    if(currentMillis >= redlightoffdelay)
    {
      digitalWrite(RELAY_2, LOW);
    }
    if(currentMillis >= electricaldelay)
    {
      if(piractive)
      {
        if(pir == HIGH)
        {
          electricalactive = true;
          http_electrical_alarm(1);
          firecrackerdelay = currentMillis + 3000;
        }
      }
    }
  }
}


void hornlogic(byte pir) {
  if(hornactive)
  {
    digitalWrite(RELAY_3, HIGH);
    if(currentMillis >= horntime)
    {
      digitalWrite(RELAY_3, LOW);
      hornactive = false;
      horndelay = currentMillis + 4000;
    }
  }
  else 
  {
    digitalWrite(RELAY_3, LOW);
    if(currentMillis >= horndelay)
    {
      if(piractive)
      {
        if(pir == HIGH)
        {
          hornactive = true;
          horntime = currentMillis + 1500;
        }
      }
    }
  }
}

void http_electrical_alarm(int value) {
  if (ethclient.connect(myserver, 80)) {  //starts client connection, checks for connection
    Serial.println("connected");
    if(value) {
      ethclient.println("POST /triggerelectricalalarm?state=1 HTTP/1.0"); //download text
    } else {
      ethclient.println("POST /triggerelectricalalarm?state=0 HTTP/1.0"); //download text
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

void http_electrical_firecracker(int value) {
  if (ethclient.connect(myserver, 80)) {  //starts client connection, checks for connection
    Serial.println("connected");
    if(value) {
      ethclient.println("POST /triggerelectricalfirecrackers?state=1 HTTP/1.0"); //download text
    } else {
      ethclient.println("POST /triggerelectricalfirecrackers?state=0 HTTP/1.0"); //download text
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
  
  if(currentMillis >= timeuntillightning1)
  {
    //Serial.print("Execute lig");
    if(numflashes1 <= 0)
    {
      //Serial.print("numflash ");
      numflashes1 = flashCount;
    }
    else
    {
      if(lightningState1 == LOW)
      {
          //Serial.print("Lightning low");
        if(currentMillis >= timeuntilnextflash1)
        {
            //Serial.print("Lightning ON");
          analogWrite(MOSSFET_1, random(flashBrightnessMin, flashBrightnessMax));
          lightningState1 = HIGH;

          flashduration1 = random(flashDurationMin, flashDurationMax) + currentMillis;
        }
      }
      else
      {
          //Serial.print("Lightning HIGH");
        if(currentMillis >= flashduration1)
        {
            //Serial.print("Lightning OFF");
          analogWrite(MOSSFET_1, 0);
          lightningState1 = LOW;
          numflashes1--;
          if(numflashes1 > 0)
          {
            timeuntilnextflash1 = random(nextFlashDelayMin, nextFlashDelayMax) + currentMillis;
          }
          else
          {
            analogWrite(MOSSFET_1, 0);
            timeuntillightning1 = random(lightningdelaymin, lightningdelaymax) + currentMillis;
          }
        }
      }
      
    }
  }
  else
  {
    analogWrite(MOSSFET_1, 0);
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
int triggerelectrical(String command) {

  // Get state from command
  int state = command.toInt();
  if(state)
  {
          electricalactive = true;
          http_electrical_alarm(1);
          firecrackerdelay = currentMillis + 3000;
  }
  else
  {
          electricalactive = false;
          http_electrical_alarm(0);
          http_electrical_firecracker(0);
          electricaldelay = currentMillis + 8000;
  }
  return 1;

}
// Trigger Pneumatic Air Horn
int triggerhorn(String command) {
  
  // Get state from command
  int state = command.toInt();

  if(state)
  {
    hornactive = true;
    horntime = currentMillis + 1500;
  }
  else
  {
    hornactive = false;
    horndelay = currentMillis + 4000;
  }
  return 1;

}
