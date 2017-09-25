
/*
  .______   .______    __    __   __    __          ___      __    __  .___________.  ______   .___  ___.      ___   .___________. __    ______   .__   __.
  |   _  \  |   _  \  |  |  |  | |  |  |  |        /   \    |  |  |  | |           | /  __  \  |   \/   |     /   \  |           ||  |  /  __  \  |  \ |  |
  |  |_)  | |  |_)  | |  |  |  | |  |__|  |       /  ^  \   |  |  |  | `---|  |----`|  |  |  | |  \  /  |    /  ^  \ `---|  |----`|  | |  |  |  | |   \|  |
  |   _  <  |      /  |  |  |  | |   __   |      /  /_\  \  |  |  |  |     |  |     |  |  |  | |  |\/|  |   /  /_\  \    |  |     |  | |  |  |  | |  . `  |
  |  |_)  | |  |\  \-.|  `--'  | |  |  |  |     /  _____  \ |  `--'  |     |  |     |  `--'  | |  |  |  |  /  _____  \   |  |     |  | |  `--'  | |  |\   |
  |______/  | _| `.__| \______/  |__|  |__|    /__/     \__\ \______/      |__|      \______/  |__|  |__| /__/     \__\  |__|     |__|  \______/  |__| \__|
  Thanks much to @corbanmailloux for providing a great framework for implementing flash/fade with HomeAssistant https://github.com/corbanmailloux/esp-mqtt-rgb-led
  To use this code you will need the following dependancies:

  - Support for the ESP8266 boards.
        - You can add it to the board manager by going to File -> Preference and pasting http://arduino.esp8266.com/stable/package_esp8266com_index.json into the Additional Board Managers URL field.
        - Next, download the ESP8266 dependancies by going to Tools -> Board -> Board Manager and searching for ESP8266 and installing it.

  - You will also need to download the follow libraries by going to Sketch -> Include Libraries -> Manage Libraries
      - DHT sensor library
      - Adafruit unified sensor
      - PubSubClient
      - ArduinoJSON

  UPDATE 16 MAY 2017 by Knutella - Fixed MQTT disconnects when wifi drops by moving around Reconnect and adding a software reset of MCU

  UPDATE 23 MAY 2017 - The MQTT_MAX_PACKET_SIZE parameter may not be setting appropriately do to a bug in the PubSub library. If the MQTT messages are not being transmitted as expected please you may need to change the MQTT_MAX_PACKET_SIZE parameter in "PubSubClient.h" directly.
*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>


/************ WIFI and MQTT INFORMATION (CHANGE THESE FOR YOUR SETUP) ******************/
#define wifi_ssid "YourSSID" //type your WIFI information inside the quotes
#define wifi_password "YourWIFIpassword"
#define mqtt_server "your.mqtt.server.ip"
#define mqtt_user "yourMQTTusername"
#define mqtt_password "yourMQTTpassword"
#define mqtt_port 1883

/************* MQTT TOPICS (change these topics as you wish)  **************************/
#define door_topic "angie/entrance-door"

/**************************** FOR OTA **************************************************/
#define SENSORNAME "nodemcu-entrance"
#define OTApassword "YouPassword" // change this to whatever password you want to use when you upload OTA
int OTAport = 8266;

/**************************** PIN DEFINITIONS ********************************************/
#define REED_PIN D0

/**************************** SENSOR DEFINITIONS *******************************************/
#define MQTT_MAX_PACKET_SIZE 512
const int BUFFER_SIZE = 300;
bool previousDoorState = true;
unsigned long previousMillis = 0;
unsigned long interval = 500; // Half second
boolean isStateChanged = false;

WiFiClient espClient;
PubSubClient client(espClient);

/********************************** START SETUP*****************************************/
void setup() {
  pinMode(REED_PIN, INPUT);

  Serial.begin(115200);
  delay(10);

  ArduinoOTA.setPort(OTAport);

  ArduinoOTA.setHostname(SENSORNAME);

  ArduinoOTA.setPassword((const char *)OTApassword);

  Serial.print("calibrating sensor ");
  for (int i = 0; i < 0; i++) {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("Starting Node named: " + String(SENSORNAME));


  setup_wifi();

  client.setServer(mqtt_server, mqtt_port);

  ArduinoOTA.onStart([]() {
    Serial.println("Starting");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IPess: ");
  Serial.println(WiFi.localIP());
  reconnect();
}


/********************************** START SETUP WIFI*****************************************/
void setup_wifi() {

  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}


/********************************** START SEND STATE*****************************************/
void sendState() {
  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["door"] = digitalRead(REED_PIN) == LOW ? true : false;
  char buffer[root.measureLength() + 1];
  root.printTo(buffer, sizeof(buffer));
  if(previousDoorState != root["door"] && !isStateChanged) {
    // Reset timer
    previousDoorState = root["door"];
    previousMillis = millis();
    isStateChanged = true;
  }
  // Only publish message whet the door is open minimum 0.5 sec
  if(isStateChanged && millis() - previousMillis > interval){ 
    if(root["door"] == previousDoorState) {
        Serial.println("Publishing");
        Serial.println(buffer);
        client.publish(door_topic, buffer, true);
      }
    isStateChanged = false;
  }
}



/********************************** START RECONNECT*****************************************/
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(SENSORNAME, mqtt_user, mqtt_password)) {
      Serial.println("connected");
      sendState();
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

/********************************** START CHECK SENSOR **********************************/
bool checkBoundSensor(float newValue, float prevValue, float maxDiff) {
  return newValue < prevValue - maxDiff || newValue > prevValue + maxDiff;
}

/********************************** START MAIN LOOP***************************************/
void loop() {

  ArduinoOTA.handle();
  
  if (!client.connected()) {
    software_Reset();
  }
  client.loop();
  sendState();
}

/****reset***/
void software_Reset() // Restarts program from beginning but does not reset the peripherals and registers
{
Serial.print("resetting");
ESP.reset(); 
}

