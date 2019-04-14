// ------------------------------------------------------------
//
//    MMRC client for controlling built-in LED
//    Copyright (C) 2019 Peter Kindström
//
//    Code to use as a starting point for MMRC clients
//
//    This program is free software: you can redistribute
//    it and/or modify it under the terms of the GNU General
//    Public License as published by the Free Software 
//    Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
// -----------------------------------------------------------
#include <PubSubClient.h>     // Library to handle MQTT communication
#include <IotWebConf.h>       // Library to take care of wifi connection & client settings
#include "MMRCsettings.h"     // Some of the MMRC client settings

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// ------------------------------------------------------------
// Variables set on the configuration web page

// Default string and number lenght
#define STRING_LEN 32
#define NUMBER_LEN 8

// Access point configuration
const char thingName[] = APNAME;                  // Initial AP name, used as SSID of the own Access Point
const char wifiInitialApPassword[] = APPASSWORD;  // Initial password, used when it creates an own Access Point

// Device configuration
char cfgMqttServer[STRING_LEN];
char cfgMqttPort[NUMBER_LEN];
char cfgMqttRetry[NUMBER_LEN];
char cfgDeviceId[STRING_LEN];
char cfgDeviceName[STRING_LEN];

// Node configuration
char cfgNode01Name[STRING_LEN];
char cfgNode01Prop01Name[STRING_LEN];
char cfgNode01Prop02Name[STRING_LEN];
char cfgSpecial01[NUMBER_LEN];
char cfgSpecial02[NUMBER_LEN];

// ------------------------------------------------------------
// Get values from MMRCsettings.h

// Node configuration
String node01Id = NODE01ID;
String node01Type = NODE01TYPE;
String node01Prop01 = NODE01PROP01;
String node01Prop01Datatype = NODE01PROP01DATATYPE;
String node01Prop02 = NODE01PROP02;
String node01Prop02Datatype = NODE01PROP02DATATYPE;

// Strings to be set after getting configuration from file
String deviceID;
String deviceName;
String node01Name;
String node01Prop01Name;
String node01Prop02Name;
String special01;
String special02;
int mqttRetry;

// ------------------------------------------------------------
// Define topic variables

// Variable for topics to subscribe to
const int nbrSubTopics = 1;
String subTopic[nbrSubTopics];

// Variable for topics to publish to
const int nbrPubTopics = 9;
String pubTopic[nbrPubTopics];
String pubTopicContent[nbrPubTopics];

// Often used topics
String pubTopicOne;
String pubTopicDeviceState;


// ------------------------------------------------------------
// IotWebConfig variables

// Indicate if it is time to reset the client or connect to MQTT
boolean needMqttConnect = false;
// boolean needReset = false;

// When CONFIG_PIN is pulled to ground on startup, the client will use the initial
// password to buld an AP. (E.g. in case of lost password)
#define CONFIG_PIN D2

// Status indicator pin.
// First it will light up (kept LOW), on Wifi connection it will blink
// and when connected to the Wifi it will turn off (kept HIGH).
#define STATUS_PIN LED_BUILTIN

// Callback method declarations
void configSaved();

// 
DNSServer dnsServer;
WebServer server(80);
IotWebConf iotWebConf(thingName, &dnsServer, &server, wifiInitialApPassword);

// ------------------------------------------------------------
// Define settings to show up on configuration web page

// Add your own configuration - MQTT
IotWebConfParameter webMqttServer = IotWebConfParameter("MQTT IP-adress", "mqttServer", cfgMqttServer, STRING_LEN);
IotWebConfParameter webMqttPort = IotWebConfParameter("MQTT-port", "mqttPort", cfgMqttPort, NUMBER_LEN);

// Add your own configuration - Device
IotWebConfParameter webDeviceId = IotWebConfParameter("Enhetens unika id", "deviceId", cfgDeviceId, STRING_LEN);
IotWebConfParameter webDeviceName = IotWebConfParameter("Enhetens namn", "deviceName", cfgDeviceName, STRING_LEN);

// Add your own configuration - Node
IotWebConfParameter webNode01Name = IotWebConfParameter("Funktionens namn", "node01Name", cfgNode01Name, STRING_LEN);
IotWebConfParameter webNode01Prop01Name = IotWebConfParameter("Egenskapens namn", "node01Prop01Name", cfgNode01Prop01Name, STRING_LEN);

// The next row does seem to cause compilation error on a Wemos D1 mini clone
// IotWebConfSeparator separator1 = IotWebConfSeparator("Test separator");


// ------------------------------------------------------------
// Other variables

// Variables to indicate if action is in progress
int actionOne = 0;    // To 'remember' than an action is in progress
int btnState = 0;     // To get two states from a momentary button

// Define which pins to use for different actions
int btnOnePin = D5;    // Pin for first button

// Uncomment next line to use built in LED on NodeMCU/Wemos/ESP8266 (which is D4)
//#define LED_BUILTIN D4


/*
 * Standard setup function
 */
void setup() {
  // Setup Arduino IDE serial monitor for "debugging"
  Serial.begin(115200);
  Serial.println();
  Serial.println("Starting setup");

  // ------------------------------------------------------------
  // Pin settings
  
  // Define build-in LED pin as output
  pinMode(LED_BUILTIN, OUTPUT);

  // Set button pin
  pinMode(btnOnePin, INPUT);

  // ------------------------------------------------------------
  // IotWebConfig start
  Serial.print("IotWebConf start...");

  // Adding up items to show on config web page
  iotWebConf.addParameter(&webMqttServer);
  iotWebConf.addParameter(&webMqttPort);
  iotWebConf.addParameter(&webDeviceId);
  iotWebConf.addParameter(&webDeviceName);
  iotWebConf.addParameter(&webNode01Name);
  iotWebConf.addParameter(&webNode01Prop01Name);
  iotWebConf.getApTimeoutParameter()->visible = true; // Show/set AP timeout at start

//  iotWebConf.setStatusPin(STATUS_PIN);
  iotWebConf.setConfigPin(CONFIG_PIN);
  Serial.println();
  Serial.print("Config pin: ");
  Serial.println(CONFIG_PIN);

  // -- Initializing the configuration
  iotWebConf.init();

/*
  // Setting default configuration
  boolean validConfig = iotWebConf.init();
  if (!validConfig)
  {
    // Set configuration default values
    mqttServerValue[0] = '\0';
    mqttUserNameValue[0] = '\0';
    mqttUserPasswordValue[0] = '\0';
  }
*/

  // Set up required URL handlers for the config web pages
  server.on("/", handleRoot);
  server.on("/config", []{ iotWebConf.handleConfig(); });
  server.onNotFound([](){ iotWebConf.handleNotFound(); });

  Serial.println("IotWebConf start...done");
  Serial.print("Assemble topics...");

  // ------------------------------------------------------------
  // Convert char to other variable types
  deviceID = String(cfgDeviceId);
  deviceName = String(cfgDeviceName);
  node01Name = String(cfgNode01Name);
  node01Prop01Name = String(cfgNode01Prop01Name);
  node01Prop02Name = String(cfgNode01Prop02Name);
  special01 = String(cfgSpecial01);
  special02 = String(cfgSpecial02);
  mqttRetry = atoi(cfgMqttRetry);

  // ------------------------------------------------------------
  // Assemble topics to subscribe and publish to

  // Subscribe
  subTopic[0] = "mmrc/"+deviceID+"/"+node01Id+"/"+node01Prop01+"/set";

  // Publish - device
  pubTopic[0] = "mmrc/"+deviceID+"/$name";
  pubTopicContent[0] = deviceName;
  pubTopic[1] = "mmrc/"+deviceID+"/$nodes";
  pubTopicContent[1] = node01Id;

  // Publish - node 01
  pubTopic[2] = "mmrc/"+deviceID+"/"+node01Id+"/$name";
  pubTopicContent[2] = node01Name;
  pubTopic[3] = "mmrc/"+deviceID+"/"+node01Id+"/$type";
  pubTopicContent[3] = node01Type;
  pubTopic[4] = "mmrc/"+deviceID+"/"+node01Id+"/$properties";
  pubTopicContent[4] = node01Prop01+","+node01Prop02;
  
  // Publish - node 01 - property 01
  pubTopic[5] = "mmrc/"+deviceID+"/"+node01Id+"/"+node01Prop01+"/$name";
  pubTopicContent[5] = node01Prop01Name;
  pubTopic[6] = "mmrc/"+deviceID+"/"+node01Id+"/"+node01Prop01+"/$datatype";
  pubTopicContent[6] = node01Prop01Datatype;

  // Publish - node 01 - property 02
  pubTopic[7] = "mmrc/"+deviceID+"/"+node01Id+"/"+node01Prop02+"/$name";
  pubTopicContent[7] = node01Prop02Name;
  pubTopic[8] = "mmrc/"+deviceID+"/"+node01Id+"/"+node01Prop02+"/$datatype";
  pubTopicContent[8] = node01Prop02Datatype;

  // Other used publish topics
  pubTopicOne = "mmrc/"+deviceID+"/"+node01Id+"/"+node01Prop01;
  pubTopicDeviceState = "mmrc/"+deviceID+"/$state";;

  Serial.println("done");
  Serial.print("Prepare MQTT...");

  // ------------------------------------------------------------
  // Prepare MQTT broker and define function to handle callbacks
  delay(2000);    // Wait for IotWebServer to start network connection
  int mqttPort = atoi(cfgMqttPort);
  mqttClient.setServer(cfgMqttServer, mqttPort);
  mqttClient.setCallback(mqttCallback);

  Serial.println("done");
  Serial.println("Setup finished");
}


/**
 * (Re)connects to MQTT broker and subscribes to one or more topics
 */
boolean mqttConnect() {
  char tmpTopic[254];

  // Loop until we're reconnected
  while (!mqttClient.connected()) {
    Serial.print("MQTT connection...");

    // Attempt to connect
    if (mqttClient.connect(cfgDeviceId)) {
      Serial.println("connected");
      Serial.print("MQTT client id: ");
      Serial.println(cfgDeviceId);

      // Subscribe to all topics
      Serial.println("Subscribing to:");
      for (int i=0; i < nbrSubTopics; i++){
        // Convert String to char* for the mqttClient.subribe() function to work
        subTopic[i].toCharArray(tmpTopic, subTopic[i].length()+1);
  
        // ... print topic
        Serial.print(" - ");
        Serial.println(tmpTopic);

        //   ... and subscribe to topic
        mqttClient.subscribe(tmpTopic);
      }
    } else {
       // Count number of connection tries
      mqttRetry += 1;

      // More than 5 tries, then start configuration portal (NOT TESTED!)
      if (mqttRetry > 5){
       Serial.println("failed, starting config portal");
       mqttRetry = 0;
       return false;
      } else {
       Serial.print("failed no=");
       Serial.print(mqttRetry);
       Serial.print(", rc=");
       Serial.print(mqttClient.state());
       Serial.println(", try again in 5 seconds");
       // Wait 5 seconds before retrying
       delay(5000);
      }
    }
  }
  return true;
  Serial.println("---");

}


/**
 * Function to handle MQTT messages sent to this device
 */
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // Don't know why this have to be done :-(
  payload[length] = '\0';

  // Print the topic
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  // Make strings and print payload
  String msg = String((char*)payload);
  String tpc = String((char*)topic);
  Serial.println(msg);

  // Send topic and payload to be interpreted
  mqttResolver(tpc, msg);
}


/**
 * Resolves messages from MQTT broker 
 */
void mqttResolver(String sbTopic, String sbPayload) {

  // Check first topic
  if (sbTopic == subTopic[0]){

    // Check message
    if (sbPayload == "0") {
      // Turn LED on and report back (via MQTT)
      setLED(0);
      mqttPublish(pubTopicOne, "0");
    }
    if (sbPayload == "1") {
      // Turn LED off and don't report back (via MQTT)
      setLED(1);
      mqttPublish(pubTopicOne, "1");
    }
  }
}


/*
 * Function to turn LED on or off
 */
void setLED (int light) {

    // Turn LED on of off
    if (light == 0) {
      digitalWrite(LED_BUILTIN, HIGH);
    } else {
      digitalWrite(LED_BUILTIN, LOW);
    }

}

/**
 * Publish messages to MQTT broker 
 */
void mqttPublish(String pbTopic, String pbPayload) {

  // Convert String to char* for the mqttClient.publish() function to work
  char msg[pbPayload.length()+1];
  pbPayload.toCharArray(msg, pbPayload.length()+1);
  char tpc[pbTopic.length()+1];
  pbTopic.toCharArray(tpc, pbTopic.length()+1);

  // Report back to pubTopic[]
  int check = mqttClient.publish(tpc, msg);

  // TODO check "check" integer to see if all went ok

  // Print information
  Serial.print("Message sent    [");
  Serial.print(pbTopic);
  Serial.print("] ");
  Serial.println(pbPayload);

  // Action 1 is executed, ready for new action
  actionOne = 0;

}

/*
 * Main program loop
 */
void loop()
{

  // Check connection to the MQTT broker. If no connection, try to reconnect
  if (needMqttConnect) {
    if (mqttConnect()) {
      needMqttConnect = false;
    }
  }
  else if ((iotWebConf.getState() == IOTWEBCONF_STATE_ONLINE) && (!mqttClient.connected())) {
    Serial.println("MQTT reconnect");
    mqttConnect();
  }

  // Wait for incoming MQTT messages
  mqttClient.loop();

  // Check for IotWebConfig actions (should be called as frequently as possible)
  iotWebConf.doLoop();


  // Check for button press
  int btnOnePress = digitalRead(btnOnePin);
  if (btnOnePress == LOW && actionOne == 0) {
    Serial.println("Button One pressed");
    delay(300);

    // Publish button press
    btnState = 1-btnState;    // Change state of Action
    if (btnState == 0) {
      actionOne = 1;                  // Action 1 is executing, new actions forbidden
      setLED(1);                      // Turn on LED
      mqttPublish(pubTopicOne, "1");  // Publish new LED state
    } else {
      actionOne = 1;                  // Action 1 is executing, new actions forbidden
      setLED(0);                      // Turn on LED
      mqttPublish(pubTopicOne, "0");  // Publish new LED state
    }
  }

}


/*
 * Function to show AP web start page
 */
void handleRoot()
{
  // -- Let IotWebConf test and handle captive portal requests.
  if (iotWebConf.handleCaptivePortal())
  {
    // -- Captive portal request were already served.
    return;
  }
  String page = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>";
  page += "<title>MMRC-inst&auml;llningar</title></head><body>";
  page += "<h1>Inst&auml;llningar</h1>";
  page += "<p>V&auml;lkommen till MMRC-enheten med namn: '";
  page += cfgDeviceId;
  page += "'</p>";
  page += "P&aring; sidan <a href='config'>Inst&auml;llningar</a> kan du best&auml;mma hur just denna MMRC-klient ska fungera.";

  page += "<p>M&ouml;jligheten att &auml;ndra dessa inst&auml;llningar &auml;r alltid tillg&auml;nglig de f&ouml;rsta ";
  page += "30";
  page += " sekunderna efter start av enheten.";

  page += "</body></html>\n";

  server.send(200, "text/html", page);
}


/*
 *  Function beeing called when wifi connection is up and running
 */
void wifiConnected() {
  // We are ready to start the MQTT connection
  needMqttConnect = true;
}
