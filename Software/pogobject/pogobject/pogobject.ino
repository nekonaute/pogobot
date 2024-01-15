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
 * configuration du wifi
 * pin D9 to GND -> normal mode
 * pin D9 to VCC -> configuration mode
 * 
 * The SSID can only make 32 caracters
 * The password is only 64 caracters
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
// Wifi configuration
#include <ESP8266WebServer.h>
#include <EEPROM.h>

//screen variables
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE); 

// WiFi configuration parameters
const char* defaultSSID = "PogobjectConfig";
const char* defaultPassword = "pogobotconfig";

// Global ssid / password value used
char ssid[32]; 
char password[64]; 

// Create an instance of a WebServer for configuration
ESP8266WebServer server_config(80);

// Check Battery voltage using multimeter & add/subtract the value
float calibration = 0.64; 

// The port to listen for incoming TCP connections
#define LISTEN_PORT 80

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
#define CONFIG_PIN D9 // put to vcc to start configuration 

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

void handleRoot() {
  String html = "<html><head><style>";
  html += "body {font-family: Arial, sans-serif; margin: 50px;}";
  html += "h1 {color: #333; text-align: center;}";
  html += "form {max-width: 400px; margin: auto;}";
  html += "input {width: 100%; padding: 10px; margin: 5px 0;}";
  html += "input[type=submit], input[type=button] {background-color: #B7B7B7; color: white;}";
  html += "</style></head><body>";
  html += "<h1>Configuration WiFi</h1>";
  html += "<form action='/configure' method='post'>";
  html += "SSID: <input type='text' name='ssid' value='" + String(ssid) + "' required><br>";
  html += "Mot de passe: <input type='password' name='password' id='password' value='" + String(password) + "' required>";
  html += "<input type='button' value='Afficher le mot de passe' onclick='togglePasswordVisibility()'><br>";
  html += "<input type='submit' value='Configurer'>";
  html += "</form>";
  html += "<script>";
  html += "function togglePasswordVisibility() {";
  html += "  var passwordInput = document.getElementById('password');";
  html += "  if (passwordInput.type === 'password') {";
  html += "    passwordInput.type = 'text';";
  html += "  } else {";
  html += "    passwordInput.type = 'password';";
  html += "  }";
  html += "}";
  html += "</script>";
  html += "</body></html>";

  server_config.send(200, "text/html", html);
}

void handleConfigure() {
  String newSSID = server_config.arg("ssid");
  String newPassword = server_config.arg("password");

  // update WIFI information
  newSSID.toCharArray(ssid, sizeof(ssid));
  newPassword.toCharArray(password, sizeof(password));

  // WRITE info to EEPROM 
  // writeWiFiCredentials();

  // Html response
  String response = "<html><body>";
  response += "<h1>Wifi info updated with success</h1>";
  response += "<p>Please move D9 to 0, the pogobect will restart now...</p>";
  response += "</body></html>";

  server_config.send(200, "text/html", response);

  delay(4000);
  ESP.restart();
}

void readWiFiCredentials() {
  EEPROM.begin(512);

  // READ EEPROM for SSID
  for (int i = 0; i < sizeof(ssid); ++i) {
    ssid[i] = EEPROM.read(i);
  }

  // READ EEPROM for password
  for (int i = 0; i < sizeof(password); ++i) {
    password[i] = EEPROM.read(sizeof(ssid) + i);
  }

  EEPROM.end();
}

void writeWiFiCredentials() {
  EEPROM.begin(512);

  // WRITE new ssid to EEPROM
  for (int i = 0; i < sizeof(ssid); ++i) {
    EEPROM.write(i, ssid[i]);
  }

  // WRITE new password to EEPROM
  for (int i = 0; i < sizeof(password); ++i) {
    EEPROM.write(sizeof(ssid) + i, password[i]);
  }

  EEPROM.commit();
  EEPROM.end();
}

void setup(void) {

  // Start Serial
  Serial.begin(BAUD_RATE);
  Serial.println("\n Starting");
  Serial.print("ESP Board MAC Address:  ");
  Serial.println(WiFi.macAddress());

  // Start screen
  u8g2.begin();

  // wifi credentials
  readWiFiCredentials();
  Serial.print("Memorized credentials : ");
  Serial.print(ssid);
  Serial.print(" , ");
  Serial.println(password);

  // look for config Pin
  pinMode(CONFIG_PIN, INPUT);
  int config_mode = digitalRead(CONFIG_PIN); 

  if (config_mode) { // 1 means start configuration
    //Start AP mode
    Serial.print("Setting wifi mode ... ");
    Serial.println(WiFi.mode(WIFI_AP) ? "Ready" : "Failed!");
    Serial.print("Setting soft-AP ... ");
    Serial.println(WiFi.softAP(defaultSSID, defaultPassword) ? "Ready" : "Failed!");

    Serial.print("Soft-AP IP address = ");
    Serial.println(WiFi.softAPIP());

    // print IP to screen
    IPaddr = IpAddress2String(WiFi.softAPIP());
    u8g2.clearBuffer();          // clear the internal memory
    u8g2.setFont(u8g2_font_ncenB08_tr); // choose a suitable font
    u8g2.drawStr(0,10,IPaddr.c_str());  // write something to the internal memory
    u8g2.sendBuffer();          // transfer internal memory to the display
    delay(1000);
    printScreen("Config mode");

    // Start config server
    server_config.on("/", HTTP_GET, handleRoot);
    server_config.on("/configure", HTTP_POST, handleConfigure);
    server_config.begin();

    while (1) {
      server_config.handleClient();
    }

  }

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
    delay(100);
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
