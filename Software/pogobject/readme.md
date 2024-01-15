# Pogobject
The Pogobject is an object that can interact with pogobots using Infrared (IR) communication.
It features:
- IR message sending capability
- REST API control
- LEDs output control
- Battery-powered capability
- A screen

It is composed of a custom PCB, a Pogobot v3 head, and a DFRobot FireBeetle ESP8266.

## Pogobject configuration
In order to configure the SSID and the password of your WIFI, move the pogobject in configuration mode by connecting D9 to VCC and restart the pogobject.

A new WIFI "PogobjectConfig" should appear with the password "pogobotconfig".
Connect to the WIFI and go to the adress http://192.168.4.1 , enter the information.
Reconnect D9 to GND and restart the pogobject. 

You should now be connected to your network.

## Pogobject Directory
This directory contains the code for the FireBeetle to be used with the Arduino IDE.
Instructions and library versions are available within the code.

Tested with the arduino IDE 1.8.13 and 2.2.1
this code is the fusion of example from the libraries
- aREST (https://github.com/marcoschwartz/aREST v2.9.7)
- SoftwareSerial
- dfRobot u8g2 (https://github.com/olikraus/u8g2 v2.27.2 or v2.34.22)
- board library FireBeetle-ESP8266 (https://raw.githubusercontent.com/DFRobot/FireBeetle-ESP8266/master/package_firebeetle8266_index.json or http://download.dfrobot.top/boards/package_DFRobot_index.json v2.3.3)
  

## Pogobject Web Control Directory
This directory contains a web app that can interact more easily with the Pogobject.
Both the Pogobject and the web app need to be on the same wifi network in order to work.
Instructions are provided on the web page.

## Pogobject API

The following API is available on the pogobject.

- Allows to access to the pogobject status. <br> 
http://X.X.X.X/

- Allows to send a cmd to the pogobot though the UART prompt. <br> 
http://X.X.X.X/cmd?params=YYY  <br> 
YYY -> all bios cmd compabible with the pogobot (without "", spaces are understand).<br> 
Example: http://X.X.X.X/cmd?params=autotest led
 
- Allows to change the PWM ratio of the pin "LED_PIN". <br> 
http://X.X.X.X/led?params=Y <br> 
Y -> [0:1024] <br>
Example: http://X.X.X.X/led?params=256
 
- Alows to reboot the pogobot for whatever is needed. <br>
http://X.X.X.X/reboot_p 

- update the battery reading. <br> 
http://X.X.X.X/read_bat 
  
