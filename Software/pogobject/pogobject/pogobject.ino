/*
 * Pogobot Object
 * 
 * Tested with the arduino IDE 1.8.13
 * this code is the fusion of example from the libraries
 * - aREST (https://github.com/marcoschwartz/aREST v2.9.7)
 * - SoftwareSerial
 * - dfRobot u8g2 (https://github.com/olikraus/u8g2 v2.27.2)
 * - board library FireBeetle-ESP8266 (https://raw.githubusercontent.com/DFRobot/FireBeetle-ESP8266/master/package_firebeetle8266_index.json v2.3.3)
 * 
 * Code realized for a FireBeetle ESP8266 (DRF0489), 0.91" oled screen I2C (DFR0647), pogobot v3 
 * 
 * Pinout :
 * pogobot
 * D3 -> TX pogobot
 * D4 -> RX pogobot
 * D2 -> RESET pogobot
 * VCC -> Bat pogobot
 * GND -> GND pogobot
 * 
 * exterior Leds 
 * D5 -> PWM to control a LED intensity
 * 
 * screen in 3.3V
 * standard  I2C connexion
 * 
 * battery verification
 * voltage divider between VCC - A0 - GND (2*100kOhms) 
 * 
 */

#include <Arduino.h>
// aREST
#include <ESP8266WiFi.h>
#include <aREST.h>
// Screen
#include <U8g2lib.h>
#include <Wire.h>
// Software Serial
#include "SoftwareSerial.h"

//screen variables
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE); 

// WiFi parameters
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_CODE";

// Check Battery voltage using multimeter & add/subtract the value
float calibration = 0.64; 

// The port to listen for incoming TCP connections
#define LISTEN_PORT           80

// Create an instance of the server
WiFiServer server(LISTEN_PORT);

// Create aREST instance
aREST rest = aREST();

// Variables to be exposed to the API
String IPaddr;
int led_on = 0;
int bat_percentage = 0;
float voltage = 0;

// Pin Definition
#define RX D3 // -> TX pogobot
#define TX D4 // -> RX pogobot
#define RESET_P D2 // -> RESET pogobot
#define LED_PIN D5 // -> PWM for Led control

// software serial definition
#define BAUD_RATE 115200
SoftwareSerial swSerial(RX, TX);

// aREST
// Declare functions to be exposed to the API
int CmdControl(String command);
int ledControl(String command);
int reboot_p(String command);
int update_bat_percent(String command);

// author apicquot from https://forum.arduino.cc/index.php?topic=228884.0
String IpAddress2String(const IPAddress& ipAddress)
{
    return String(ipAddress[0]) + String(".") +
           String(ipAddress[1]) + String(".") +
           String(ipAddress[2]) + String(".") +
           String(ipAddress[3]);
}

// general function to print on the screen
void printScreen(String text)
{
    u8g2.clearBuffer();          // clear the internal memory
    u8g2.setFont(u8g2_font_ncenB08_tr); // choose a suitable font
    u8g2.drawStr(0,30,text.c_str());  // write something to the internal memory
    u8g2.drawStr(0,10,IPaddr.c_str());  // write something to the internal memory
    u8g2.sendBuffer();          // transfer internal memory to the display
    delay(1000);
    Serial.println(text);
} 

// from https://github.com/radishlogic/MapFloat/blob/master/src/MapFloat.cpp
float mapFloat(float value, float fromLow, float fromHigh, float toLow, float toHigh) {
  return (value - fromLow) * (toHigh - toLow) / (fromHigh - fromLow) + toLow; 
}


void setup(void) {

  // Start Serial
  Serial.begin(BAUD_RATE);
  Serial.println("\n Starting");
  Serial.print("ESP Board MAC Address:  ");
  Serial.println(WiFi.macAddress());

  // Start screen
  u8g2.begin();
  printScreen("Waiting for wifi");


  // pin mode definition
  pinMode(LED_PIN, OUTPUT);
  pinMode(RESET_P, OUTPUT);

  // Wifi connection
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  printScreen("Wifi ok");
  Serial.println("\n Wifi connected");
  
  // Start the server
  server.begin();

  // Print the IP address
  Serial.println(WiFi.localIP());

  // Print the IP address on  the screen
  IPaddr = IpAddress2String(WiFi.localIP());
  u8g2.clearBuffer();          // clear the internal memory
  u8g2.setFont(u8g2_font_ncenB08_tr); // choose a suitable font
  u8g2.drawStr(0,10,IPaddr.c_str());  // write something to the internal memory
  u8g2.sendBuffer();          // transfer internal memory to the display
  delay(1000);  

  //Check Battery
  update_bat_percent("none");
  Serial.println(bat_percentage);

  // Init variables and expose them to REST API
  rest.variable("IPaddr",&IPaddr);
  rest.variable("LedOn",&led_on);
  rest.variable("bat_percentage",&bat_percentage);
  rest.variable("bat_voltage",&voltage);

  // Function to be exposed
  // http://X.X.X.X/cmd?params=cmd  -> all cmd (without ", spaces are understand) compabible with the pogobot "auto_test led" for example
  // allows to send a cmd to the pogobot though the UART prompt
  rest.function((char*)"cmd",CmdControl);
  // http://X.X.X.X/led?params=Y  -> Y [0:1024]
  // allows to change the pwm ratio of the pin "LED_PIN"
  rest.function((char*)"led",ledControl);
  // http://X.X.X.X/reboot_p 
  // alows to reboot the pogobot for whatever is needed  
  rest.function((char*)"reboot_p",reboot_p);
  // http://X.X.X.X/read_bat 
  // update the battery reading 
  rest.function((char*)"read_bat",update_bat_percent);

  // Give name & ID to the device (ID should be 6 characters long)
  rest.set_id("1"); // Allows to give a unique ID to the pogobot object
  rest.set_name((char*)"pogObject");

  // Start the Software Serial to communicate with the pogobot
  swSerial.begin(BAUD_RATE);
  
  // keep pogobot alive
  digitalWrite(RESET_P,HIGH);
  
}

void loop(void) {

  //Check Battery
  //update_bat_percent();

  // Handle REST calls
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
  while(!client.available()){
    delay(1);
  }
  rest.handle(client);

}

// function executed when a cmd on the pogobot is asked
int CmdControl(String command) {

  //print on screen
  printScreen(command);
  
  //send by serial
  swSerial.print(command+"\n");
  
  return 1;
}


// function to recover the battery voltage
int update_bat_percent(String command) {
  //recover sensor value
  int sensorValue = analogRead(A0);
  //multiply by two as voltage divider network is 100K & 100K Resistor
  voltage = (((sensorValue * 3.3) / 1024) * 2 + calibration); 
  //2.8V as Battery Cut off Voltage & 4.2V as Maximum Voltage
  bat_percentage = mapFloat(voltage, 3.0, 4.2, 0, 100); 

  return 1;
}

// function executed when the led pwm is changed
int ledControl(String command) {

  // Get state from command
  int state = command.toInt();
  led_on = state;
  analogWrite(LED_PIN,state);

  String text = "led at : " + command;
  printScreen(text);
  
  return 1;
}

// function executed when a reboot of the pogobot is asked
int reboot_p(String command) {

  // reboot command
  digitalWrite(RESET_P,LOW);
  delay(1000);
  digitalWrite(RESET_P,HIGH);

  printScreen("reboot pogobot");
  
  return 1;
}
