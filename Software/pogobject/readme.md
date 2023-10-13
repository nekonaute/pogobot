# Pogobject
The Pogobject is an object that can interact with pogobots using Infrared (IR) communication.
It features:
- IR message sending capability
- REST API control
- LEDs output control
- Battery-powered capability
- A screen

It is composed of a custom PCB, a Pogobot v3 head, and a DFRobot FireBeetle ESP8266.

## Pogobject Directory
This directory contains the code for the FireBeetle to be used with the Arduino IDE.
Instructions and library versions are available within the code.

Tested with the arduino IDE 1.8.13
this code is the fusion of example from the libraries
- aREST (https://github.com/marcoschwartz/aREST v2.9.7)
- SoftwareSerial
- dfRobot u8g2 (https://github.com/olikraus/u8g2 v2.27.2)
- board library FireBeetle-ESP8266 (https://raw.githubusercontent.com/DFRobot/FireBeetle-ESP8266/master/package_firebeetle8266_index.json v2.3.3)
  or http://download.dfrobot.top/boards/package_DFRobot_index.json 
  

## Pogobject Web Control Directory
This directory contains a web app that can interact more easily with the Pogobject.
Both the Pogobject and the web app need to be on the same wifi network in order to work.
Instructions are provided on the web page.
