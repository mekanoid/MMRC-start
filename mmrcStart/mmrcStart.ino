// --------------------------------------------------------------------------------------------------
//
//    MMRC client for controlling a LED
//    Copyright (C) 2019 Peter Kindström
//
//    Code to use as a starting point for MMRC clients
//
//    This program is free software: you can redistribute it
//    and/or modify it under the terms of the GNU General
//    Public License as published by the Free Software 
//    Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//     Uses and have been tested witch the following libraries
//      PubSubClient v2.7.0 by Nick O'Leary - https://pubsubclient.knolleary.net/
//      IotWebConf   v2.3.0 by Balazs Kelemen - https://github.com/prampec/IotWebConf
//      EasyButton   v1.1.1 by Evert Arias    - https://github.com/evert-arias/EasyButton
//  
// --------------------------------------------------------------------------------------------------
// Include all libraries
#include <PubSubClient.h>     // Library to handle MQTT communication
#include <IotWebConf.h>       // Library to take care of wifi connection & client settings
#include <EasyButton.h>       // Handle button presses
#include "mmrcSettings.h"     // Some of the MMRC client settings

// Make objects for the PubSubClient
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// Make object for pushbutton
EasyButton button(BUTTON_PIN, 100, false, true);

// --------------------------------------------------------------------------------------------------
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
char cfgDeviceId[STRING_LEN];
char cfgDeviceName[STRING_LEN];


// --------------------------------------------------------------------------------------------------
// Define topic variables

// Variable for topics to subscribe to
const int nbrSubTopics = 1;
String subTopic[nbrSubTopics];

// Variable for topics to publish to
const int nbrPubTopics = 7;
String pubTopic[nbrPubTopics];
String pubTopicContent[nbrPubTopics];

// Often used topics
String pubLedTopic;           // Topic showing the state of status LED
String pubBtnTopic;           // Topic showing the state of pushbutton
String pubBtnContent;         // Initial state of button
String pubDeviceStateTopic;   // Topic showing the state of device

const byte NORETAIN = 0;
const byte RETAIN = 1;

// --------------------------------------------------------------------------------------------------
// IotWebConfig variables

// Indicate if it is time to reset the client or connect to MQTT
boolean needMqttConnect = false;
// boolean needReset = false;

// Callback method declarations
void configSaved();

// Make objects for IotWebConf
DNSServer dnsServer;
WebServer server(80);
IotWebConf iotWebConf(thingName, &dnsServer, &server, wifiInitialApPassword);

// --------------------------------------------------------------------------------------------------
// Define settings to show up on configuration web page

// Add a new parameter section on the settings web page
IotWebConfSeparator separator1 = IotWebConfSeparator("MQTT-inställningar");

// Add configuration for MQTT
IotWebConfParameter webMqttServer = IotWebConfParameter("MQTT IP-adress", "mqttServer", cfgMqttServer, STRING_LEN);
IotWebConfParameter webMqttPort = IotWebConfParameter("MQTT-port", "mqttPort", cfgMqttPort, NUMBER_LEN);

// Add a new parameter section on the settings web page
IotWebConfSeparator separator2 = IotWebConfSeparator("Enhetens inställningar");

// Add configuration for Device
IotWebConfParameter webDeviceId = IotWebConfParameter("Enhetens unika id", "deviceId", cfgDeviceId, STRING_LEN);
IotWebConfParameter webDeviceName = IotWebConfParameter("Enhetens namn", "deviceName", cfgDeviceName, STRING_LEN);

// --------------------------------------------------------------------------------------------------
// Other variables

// Strings to be set after getting configuration from file
String deviceID;
String deviceName;

// Variables to indicate if action is in progress
int actionOne = 0;    // To 'remember' than an action is in progress
int btnState = 0;     // To get two states from a momentary button

// Debug
byte debug = 1;               // Set to "1" for debug messages in Serial monitor (9600 baud)
String dbText = "Main   : ";  // Debug text. Occurs first in every debug output


// --------------------------------------------------------------------------------------------------
//  Standard setup function
// --------------------------------------------------------------------------------------------------
void setup() {
  // Setup Arduino IDE serial monitor for "debugging"
  if (debug == 1) {Serial.begin(9600);Serial.println("");}
  if (debug == 1) {Serial.println(dbText+"Starting setup");}

  // ------------------------------------------------------------------------------------------------
  // Pin settings
  
  // Define build-in LED pin as output
  //  if (debug == 1) {Serial.println(dbText+"Config pin = "+CONFIG_PIN);}
  pinMode(LED_PIN, OUTPUT);


  // ------------------------------------------------------------------------------------------------
  // Button start
  if (debug == 1) {Serial.println(dbText+"Button setup");}
  if (debug == 1) {Serial.println(dbText+"Button pin = "+BUTTON_PIN);}
  pinMode(BUTTON_PIN, INPUT);

  // Initialize the button
  button.begin();

  // Add the callback function to be called when the button is pressed
  button.onPressed(btnPressed);
  
  // ------------------------------------------------------------------------------------------------
  // IotWebConfig start
  if (debug == 1) {Serial.println(dbText+"IotWebConf setup");}

  // Adding up items to show on config web page
  iotWebConf.addParameter(&separator1);
  iotWebConf.addParameter(&webMqttServer);
  iotWebConf.addParameter(&webMqttPort);
  iotWebConf.addParameter(&separator2);
  iotWebConf.addParameter(&webDeviceId);
  iotWebConf.addParameter(&webDeviceName);

// Show/set AP timeout on web page
// iotWebConf.getApTimeoutParameter()->visible = true;

//  iotWebConf.setStatusPin(STATUS_PIN);
//  if (debug == 1) {Serial.println(dbText+"Status pin = "+STATUS_PIN);}

//  iotWebConf.setConfigPin(CONFIG_PIN);
//  if (debug == 1) {Serial.println(dbText+"Config pin = "+CONFIG_PIN);}

  iotWebConf.setConfigSavedCallback(&configSaved);
  iotWebConf.setWifiConnectionCallback(&wifiConnected);

  // Setting default configuration
  boolean validConfig = iotWebConf.init();
  if (!validConfig)
  {
    // Set configuration default values
    deviceName = APNAME;
    String tmpNo = String(random(2147483647));
    deviceID = deviceName+"-"+tmpNo;
  } else {
    deviceID = String(cfgDeviceId);
    deviceName = String(cfgDeviceName);
  }

  // Set up required URL handlers for the config web pages
  server.on("/", handleRoot);
  server.on("/config", []{ iotWebConf.handleConfig(); });
  server.onNotFound([](){ iotWebConf.handleNotFound(); });


  // ------------------------------------------------------------------------------------------------
  // Assemble topics to subscribe and publish to
  if (debug == 1) {Serial.println(dbText+"Topics setup");}

  // Subscribe
  subTopic[0] = "mmrc/"+deviceID+"/button01/push/set";

  // Publish - device
  pubTopic[0] = "mmrc/"+deviceID+"/$name";
  pubTopicContent[0] = deviceName;
  pubTopic[1] = "mmrc/"+deviceID+"/$nodes";
  pubTopicContent[1] = "button01";

  // Publish - node 01
  pubTopic[2] = "mmrc/"+deviceID+"/button01/$name";
  pubTopicContent[2] = "Tryckknapp med status";
  pubTopic[3] = "mmrc/"+deviceID+"/button01/$type";
  pubTopicContent[3] = "pushbutton";
  pubTopic[4] = "mmrc/"+deviceID+"/button01/$properties";
  pubTopicContent[4] = "push,status";
  
  // Publish - node 01 - property 01
  pubTopic[5] = "mmrc/"+deviceID+"/button01/push/$name";
  pubTopicContent[5] = "Knapptryck";
  pubTopic[6] = "mmrc/"+deviceID+"/button/push/$datatype";
  pubTopicContent[6] = "string";

  // Publish - node 01 - property 02
  pubTopic[5] = "mmrc/"+deviceID+"/button01/status/$name";
  pubTopicContent[5] = "Lysdiod";
  pubTopic[6] = "mmrc/"+deviceID+"/button/status/$datatype";
  pubTopicContent[6] = "string";

  // Other used publish topics
  pubLedTopic = "mmrc/"+deviceID+"/button01/status";
  pubBtnTopic = "mmrc/"+deviceID+"/button01/push";
  pubBtnContent = "zero";
  pubDeviceStateTopic = "mmrc/"+deviceID+"/$state";

  // ------------------------------------------------------------------------------------------------
  // Prepare MQTT broker and define function to handle callbacks
  if (debug == 1) {Serial.println(dbText+"MQTT setup");}
  delay(2000);    // Wait for IotWebServer to start network connection
  int mqttPort = atoi(cfgMqttPort);
  mqttClient.setServer(cfgMqttServer, mqttPort);
  mqttClient.setCallback(mqttCallback);

  if (debug == 1) {Serial.println(dbText+"Setup done!");}

}


// --------------------------------------------------------------------------------------------------
// (Re)connects to MQTT broker and subscribes to one or more topics
// --------------------------------------------------------------------------------------------------
boolean mqttConnect() {
  char tmpTopic[254];
  char tmpContent[254];
  char tmpID[deviceID.length()];    // For converting deviceID
  char* tmpMessage = "lost";        // Status message in Last Will
  
  // Convert String to char* for last will message
  deviceID.toCharArray(tmpID, deviceID.length()+1);
  pubDeviceStateTopic.toCharArray(tmpTopic, pubDeviceStateTopic.length()+1);
  
  // Loop until we're reconnected
  while (!mqttClient.connected()) {
  if (debug == 1) {Serial.print(dbText+"MQTT connection...");}

    // Attempt to connect
    // boolean connect (tmpID, pubDeviceStateTopic, willQoS, willRetain, willMessage)
    if (mqttClient.connect(tmpID,tmpTopic,0,true,tmpMessage)) {
      if (debug == 1) {Serial.println("connected");}
      if (debug == 1) {Serial.print(dbText+"MQTT client id = ");}
      if (debug == 1) {Serial.println(cfgDeviceId);}

      // Subscribe to all topics
      if (debug == 1) {Serial.println(dbText+"Subscribing to:");}
      for (int i=0; i < nbrSubTopics; i++){
        // Convert String to char* for the mqttClient.subribe() function to work
        subTopic[i].toCharArray(tmpTopic, subTopic[i].length()+1);
  
        // ... print topic
        if (debug == 1) {Serial.println(dbText+" - "+tmpTopic);}

        //   ... and subscribe to topic
        mqttClient.subscribe(tmpTopic);
      }

      // Publish to all topics
      if (debug == 1) {Serial.println(dbText+"Publishing to:");}
      for (int i=0; i < nbrPubTopics; i++){
        // Convert String to char* for the mqttClient.publish() function to work
        pubTopic[i].toCharArray(tmpTopic, pubTopic[i].length()+1);
        pubTopicContent[i].toCharArray(tmpContent, pubTopicContent[i].length()+1);

        // ... print topic
        if (debug == 1) {Serial.print(dbText+" - "+tmpTopic);}
        if (debug == 1) {Serial.print(" = ");}
        if (debug == 1) {Serial.println(tmpContent);}

        // ... and subscribe to topic
        mqttClient.publish(tmpTopic, tmpContent, true);
      
      }
     
    } else {
      // Show why the connection failed
      if (debug == 1) {Serial.print(dbText+"failed, rc=");}
      if (debug == 1) {Serial.print(mqttClient.state());}
      if (debug == 1) {Serial.println(", try again in 5 seconds");}

      // Wait 5 seconds before retrying
      delay(5000);
     
    }
  }

  // Set device status to "ready"
  mqttPublish(pubBtnTopic, "zero", RETAIN);
  mqttPublish(pubDeviceStateTopic, "ready", RETAIN);
  return true;

}


// --------------------------------------------------------------------------------------------------
//  Function to handle MQTT messages sent to this device
// --------------------------------------------------------------------------------------------------
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // Don't know why this have to be done :-(
  payload[length] = '\0';

  // Make strings
  String msg = String((char*)payload);
  String tpc = String((char*)topic);

  // Print the topic and payload
  if (debug == 1) {Serial.println(dbText+"Recieved: "+tpc+" = "+msg);}

  // Check for "button01/push/set" command
  if (tpc == subTopic[0]) {
    if (msg == "one") {
      btnPressed();
    }
  }

}


// --------------------------------------------------------------------------------------------------
//  Publish message to MQTT broker
// --------------------------------------------------------------------------------------------------
void mqttPublish(String pbTopic, String pbPayload, byte retain) {

  // Convert String to char* for the mqttClient.publish() function to work
  char msg[pbPayload.length()+1];
  pbPayload.toCharArray(msg, pbPayload.length()+1);
  char tpc[pbTopic.length()+1];
  pbTopic.toCharArray(tpc, pbTopic.length()+1);

  // Report back to pubTopic[]
  int check = mqttClient.publish(tpc, msg, retain);

  // TODO check "check" integer to see if all went ok

  // Print information
  if (debug == 1) {Serial.println(dbText+"Sending: "+pbTopic+" = "+pbPayload);}

  // Action 1 is executed, ready for new action
  actionOne = 0;

}


// --------------------------------------------------------------------------------------------------
//  Function that gets called when the button is pressed
// --------------------------------------------------------------------------------------------------
void btnPressed () {
    if (debug == 1) {Serial.println(dbText+"Button pressed");}

    // Toggle button function
    if (btnState == 0) {
      if (debug == 1) {Serial.println(dbText+"Set to HIGH");}
      digitalWrite(LED_PIN, HIGH);
      mqttPublish(pubLedTopic, "high", RETAIN);  // Publish new LED state
      btnState = 1;
    } else {
      if (debug == 1) {Serial.println(dbText+"Set to LOW");}
      digitalWrite(LED_PIN, LOW);
      mqttPublish(pubLedTopic, "low", RETAIN);  // Publish new LED state
      btnState = 0;
    }

    mqttPublish(pubBtnTopic, "one", NORETAIN);  // Publish new button state
}


// --------------------------------------------------------------------------------------------------
// Main program loop
// --------------------------------------------------------------------------------------------------
void loop()
{

  // Check connection to the MQTT broker. If no connection, try to reconnect
  if (needMqttConnect) {
    if (mqttConnect()) {
      needMqttConnect = false;
    }
  }
  else if ((iotWebConf.getState() == IOTWEBCONF_STATE_ONLINE) && (!mqttClient.connected())) {
    if (debug == 1) {Serial.println(dbText+"MQTT reconnect");}
    mqttConnect();
  }


  // Run repetitive jobs
  mqttClient.loop();        // Wait for incoming MQTT messages
  iotWebConf.doLoop();      // Check for IotWebConfig actions (should be called as frequently as possible)
  button.read();            // Check for pushbutton pressed

}


// --------------------------------------------------------------------------------------------------
//  Function to show AP web start page
// --------------------------------------------------------------------------------------------------
void handleRoot()
{
  // -- Let IotWebConf test and handle captive portal requests.
  if (iotWebConf.handleCaptivePortal())
  {
    // -- Captive portal request were already served.
    return;
  }

  // Assemble web page content
  String page = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" ";
  page += "content=\"width=device-width, initial-scale=1, user-scalable=no\"/>";

  page += "<title>MMRC-inst&auml;llningar</title></head><body>";
  page += "<h1>Inst&auml;llningar</h1>";
  page += "<p>V&auml;lkommen till MMRC-enheten med namn: '";
  page += cfgDeviceId;
  page += "'</p>";

  page += "<p>P&aring; sidan <a href='config'>Inst&auml;llningar</a> kan du ";
  page += "best&auml;mma hur just denna MMRC-klient ska fungera.</p>";

  page += "<p>M&ouml;jligheten att &auml;ndra dessa inst&auml;llningar ";
  page += "&auml;r alltid tillg&auml;nglig de f&ouml;rsta ";
  page += "30";
  page += " sekunderna efter start av enheten.</p>";

  page += "</body></html>\n";

  // Show web start page
  server.send(200, "text/html", page);
}


// --------------------------------------------------------------------------------------------------
//  Function beeing called when wifi connection is up and running
// --------------------------------------------------------------------------------------------------
void wifiConnected() {
  // We are ready to start the MQTT connection
  needMqttConnect = true;
}

// --------------------------------------------------------------------------------------------------
//  Function that gets called when web page config has been saved
// --------------------------------------------------------------------------------------------------
void configSaved()
{
  if (debug == 1) {Serial.println(dbText+"IotWebConf config saved");}
  deviceID = String(cfgDeviceId);
  deviceName = String(cfgDeviceName);
}
