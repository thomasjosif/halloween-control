/*
  Halloween Main Controller 01
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
byte mac[] = { 0x5A, 0x57, 0x74, 0x87, 0xAA, 0xEF };

// IP address in case DHCP fails
IPAddress ip(192,168,0,20);

char serveraddr = "192.168.1.110";


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
int triggerelectricalfirecrackers(String command);
int triggerelectricalalarm(String command);

/////////////////
// PIN MAPPING //
/////////////////

// RELAYS
int RELAY_1 = 3; // Electrical Box Warning Light
int RELAY_2 = 4; // Electrical Box Inside Light

// Variables

unsigned long currentMillis = 0;  

unsigned long firecrackerdelay = 0;
bool firecracker = false;
bool alarm = false;

//void http_communicate_send(String ip, 
void setup(void)
{
  // Start Serial
  Serial.begin(115200);

  // Function to be exposed
  rest.function("triggerelectricalalarm",triggerelectricalalarm);
  rest.function("triggerelectricalfirecrackers",triggerelectricalfirecrackers);

  // Give name & ID to the device (ID should be 6 characters long)
  rest.set_id("001");
  rest.set_name("main_control");

  /////////////////
  // PIN CONFIG  //
  /////////////////
  pinMode(RELAY_1, OUTPUT);
  pinMode(RELAY_2, OUTPUT);

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

  firecrackerlogic();
  alarmlogic();

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

}


void alarmlogic() {
  if(alarm)
  {
    digitalWrite(RELAY_2, HIGH);
  }
  else 
  {
    digitalWrite(RELAY_2, LOW);
  }
}


void firecrackerlogic() {
  if(firecracker)
  {
    digitalWrite(RELAY_1, HIGH);
    if(currentMillis >= firecrackerdelay)
    {
      digitalWrite(RELAY_1, LOW);
      firecracker = false;
    }
  }
  else 
  {
    digitalWrite(RELAY_1, LOW);
  }
}


// Trigger Electrical box alarm light
int triggerelectricalalarm(String command) {

  // Get state from command
  int state = command.toInt();
  if(state)
  {
    alarm = true;
  }
  else
  {
    alarm = false;
  }
  return 1;

}
// Trigger Pneumatic Air Horn
int triggerelectricalfirecrackers(String command) {
  
  // Get state from command
  int state = command.toInt();

  if(state)
  {
    firecracker = true;
    firecrackerdelay = currentMillis + 2000;
  }
  else
  {
    firecracker = false;
  }
  return 1;

}
