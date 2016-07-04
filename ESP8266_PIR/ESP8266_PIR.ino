/*
 *  ESP8266_PIR 
 *  Motion Sensor using Adafruit HUZZAH ESP8266 and PIR Sensor
 *  Sends events to IFTTT for EMAIL or SMS script
 *  
 *  Requires HTTPS SSL.  To get SHA1 Fingerprint:
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
 *  version 1.0 2016-06-27 R.Grokett
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

const char* host     = "maker.ifttt.com";
const int port       = 443;
const char* api_key  = "{YOUR_IFTTT_API_KEY}"; // Your API KEY from https://ifttt.com/maker
const char* event    = "pirtrigger";             // Your IFTTT Event Name

const char* SHA1Fingerprint="A9 81 E1 35 B3 7F 81 B9 87 9D 11 DD 48 55 43 2C 8F C3 EC 87";  // See SHA1 comment above 
bool verifyCert = false; // Select true if you want SSL certificate validation


int LED = 0;          // GPIO 0 (built-in LED)
int PIRpin = 14;      // GPIO 14 (PIR Sensor)
int motionState;      // cache for current motion state value
int MOTION_DELAY = 15; // Delay in seconds between events to keep from flooding IFTTT & emails


// INITIALIZE and Connect to WiFi
void setup() {
  Serial.begin(115200);
  delay(10);

  // prepare Geiger input pin
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

  // Set Auto Modem Sleep to save power
  wifi_fpm_set_sleep_type(MODEM_SLEEP_T);

}


// SEND HTTPS GET TO IFTTT
void sendEvent()
{
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
  Serial.println("going to light sleep / 0xFFFFFFF");
  delay(100);
  wifi_station_disconnect();
  wifi_set_opmode(NULL_MODE);
  wifi_fpm_set_sleep_type(LIGHT_SLEEP_T);
  wifi_fpm_open();
  gpio_pin_wakeup_enable(GPIO_ID_PIN(PIRpin), GPIO_PIN_INTR_LOLEVEL);
  wifi_fpm_do_sleep(0xFFFFFFF);
  delay(200);
}

// POWER UP WIFI TO TRANSMIT
void powerUp()
{
  gpio_pin_wakeup_disable();
  Serial.println("wake up / 0xFFFFFFF");
  wifi_fpm_close();
  wifi_set_opmode(STATION_MODE);
  wifi_station_connect();
  //wifi_connect();
  delay(100);
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
}


