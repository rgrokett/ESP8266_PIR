/*
 *  ESP8266_PIR 
 *  Motion Sensor using Adafruit HUZZAH ESP8266 and PIR Sensor
 *  Sends events to IFTTT for EMAIL or SMS script
 *  Project Documentation & files at 
 *  https://github.com/rgrokett/ESP8266_PIR
 *  
 *  IFTTT requires HTTPS SSL. You can elect to verify their cert or just accept it by setting verifyCert variable.
 *  Default is "false" for initial use.
 *  If needed, update IFTTT SHA1 Fingerprint used for cert verification:
 *  $ openssl s_client -servername maker.ifttt.com -connect maker.ifttt.com:443 | openssl x509 -fingerprint -noout
 *  (Replace colons with spaces in result)
 *  
 *  Create a IFTTT Recipe using the Maker Channel to trigger a Send Email or Send SMS Action.
 *  
 *  HTTPSRedirect is available from GitHub at:
 *  https://github.com/electronicsguy/ESP8266/tree/master/HTTPSRedirect
 *  
 *  The ESP8266WiFi library should include WiFiClientSecure already in it
 *  
 *  This version also contains power saving features, turning off Wifi between detects
 *  Note that some 5V batteries turn themselves off if no load. 
 *  Set "FiveVoltBat" to true if needed.
 *
 *  version 1.2 2016-09-16 R.Grokett
 */

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include "HTTPSRedirect.h"

extern "C" {
  #include "gpio.h"
}

extern "C" {
#include "user_interface.h" 
}

const char* ssid     = "{YOUR_WIFI_SSID}";  // Your WiFi SSID
const char* password = "{YOUR_WIFI_PWD}";   // Your WiFi Password
const char* api_key  = "{YOUR_IFTTT_API_KEY}"; // Your API KEY from https://ifttt.com/maker
const char* event    = "pirtrigger";             // Your IFTTT Event Name 

const char* host     = "maker.ifttt.com";
const int port       = 443;

const char* SHA1Fingerprint="A9 81 E1 35 B3 7F 81 B9 87 9D 11 DD 48 55 43 2C 8F C3 EC 87";  // See SHA1 comment above 
bool verifyCert = false; // Select false if you do NOT want SSL certificate validation


int LED = 0;          // GPIO 0 (built-in LED)
int PIRpin = 14;      // GPIO 14 (PIR Sensor)
int MOTION_DELAY = 15; // Delay in seconds between events to keep from flooding IFTTT & emails
bool POWER_SAVE = true; // Save power by turning off Wifi. 
bool FiveVoltBat = false; // Set to true if 5V bat automatically turns off.

int loops = 0;        // power save "timer"
int motionState;      // cache for current motion state value

// INITIALIZE and Connect to WiFi
void setup() {
  Serial.begin(115200);
  delay(10);

  // prepare PIR input pin
  pinMode(PIRpin, INPUT);
  digitalWrite(PIRpin, HIGH);

  // prepare onboard LED
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);  
  
  // We start by connecting to a WiFi network

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Blink onboard LED to signify its connected
  blink();
  blink();
  blink();
  blink(); 
  
  powerSave();  // Turn off Wifi
}


// SEND HTTPS GET TO IFTTT
void sendEvent()
{
    powerUp();  // Turn on Wifi
    Serial.print("connecting to ");
    Serial.println(host);
  
    // Use WiFiClient class to create TCP connections
    // IFTTT does a 302 Redirect, so we need HTTPSRedirect 
    HTTPSRedirect client(port);
    if (!client.connect(host, port)) {
      Serial.println("connection failed");
      return;
    }

	if (verifyCert) {
	  if (!client.verify(SHA1Fingerprint, host)) {
        Serial.println("certificate doesn't match. will not send message.");
        return;
	  } 
	}
  
    // We now create a URI for the request
    String url = "/trigger/"+String(event)+"/with/key/"+String(api_key);
    Serial.print("Requesting URL: https://");
    Serial.println(host+url);
  
    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" + 
               "Connection: close\r\n\r\n");
    delay(10);
  
    // Read all the lines of the reply from server and print them to Serial
    while(client.available()){
      String line = client.readStringUntil('\r');
      Serial.print(line);
    }
  
    Serial.println();
    Serial.println("closing connection");

    // Keep from flooding IFTTT and emails from quick triggers
    delay(MOTION_DELAY * 1000);
    
    powerSave();  // Turn off Wifi

}

// Blink the LED 
void blink() {
  digitalWrite(LED, LOW);
  delay(100); 
  digitalWrite(LED, HIGH);
  delay(100);
}

// POWER DOWN WIFI TO SAVE BATTERY
void powerSave()
{
  if (!POWER_SAVE) { return; }
  Serial.println("going to modem sleep");
  delay(100);
  WiFi.forceSleepBegin(0);
  delay(200);
}

// POWER UP WIFI TO TRANSMIT
void powerUp()
{
  if (!POWER_SAVE) { return; }
  Serial.println("wake up modem");
  WiFi.forceSleepWake();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(WiFi.localIP());
}

// POWER CHECK THE BATTERY
// Some 5V batteries turn off automatically
// This should keep them active
void powerCheck()
{
  if (!FiveVoltBat) { return; }
  if (loops > 30) // pulse the battery every 15 sec or so.
  {
      Serial.println("battery check");
      WiFi.forceSleepWake();
      delay(100);
      WiFi.forceSleepBegin(0);   
      delay(100); 
      blink();
      loops = 0;
  }
  loops = loops + 1;
}


// MAIN LOOP
void loop() {
    delay(500); 

    // Read PIR motion sensor
    int newState = digitalRead(PIRpin);

    // Trigger only if state has changed from last detection
    if (newState != motionState)
    {
      motionState = newState;
      Serial.print("State changed: ");
      Serial.println(motionState);
      if (motionState == 0)   // PIR sensor is normally High (1) so zero (0) means motion detected
      {
        sendEvent();
      }
    }

    digitalWrite(LED, 1 - motionState);   // LED light motion indication 

    powerCheck();  // Keeps battery active
}


