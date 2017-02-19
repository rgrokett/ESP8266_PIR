# ESP8266_PIR
ESP8266 Motion Detector to IFTTT

We just installed a Cat Door in our garage and I wanted to see how many times per day (actually night) our cat went in and out the door. We could tell the cat was using the door as we would find it outside sometimes and inside sometimes. 

So, breaking out my parts box, I found a PIR motion sensor, an ESP8266 and a 5V battery. 

I used the Adafruit HUZZAH ESP8266 as it has 5V regulator for powering the 3.3v ESP as well as good tutorials for initially getting it set up. I also used the Arduino IDE with the ESP8266 library, since I was already quite familiar with using it with the Huzzah ESP8266. 

I decided to interface this to IFTTT (www.ifttt.com) to trigger any number of types of events. Initially, just an email each time motion was detected. 

Note that IFTTT requires HTTPS SSL encryption. Thus this project includes code for that.

See ESP8266_PIR.pdf for details

### See Also: Deep Sleep Version
There is another version of this project to use ESP8266 Deep Sleep mode to significantly save battery power. BUT requires extra hardware, so more complex.  See https://github.com/rgrokett/ESP8266_PIRv2 

