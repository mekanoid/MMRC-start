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
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include <DNSServer.h>            // Local DNS server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     // Local web server used to serve the configuration portal
#include <WiFiManager.h>

#include "MMRCsettings.h"

WiFiClient wifiClient;
PubSubClient client(wifiClient);

// ------------------------------------------------------------
// Get values from MMRCsettings.h and define default configuration

// Device configuration
// If the same variable is found in "config.json" that value will be used instead
char cfgMqttServer[20] = BROKERIP;
char cfgMqttPort[6] = BROKERPORT;
char cfgMqttRetry[6] = BROKERRETRY;
char cfgDeviceId[40] = DEVICEID;
char cfgDeviceName[40] = DEVICENAME;

// Node configuration
// If the same variable is found in "config.json" that value will be used instead
char cfgNode01Name[40] = NODE01NAME;
char cfgNode01Prop01Name[40] = NODE01PROP01NAME;
char cfgNode01Prop02Name[40] = NODE01PROP02NAME;
char cfgSpecial01[5] = SPECIAL01;
char cfgSpecial02[5] = SPECIAL02;

// Node configuration
// Will NOT be overwritten by "config.json"
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
// Other variables

// Variables for client info
char* clientID;      // Id/name for this specific client, shown i MQTT and router
  
// Variables to indicate if action is in progress
int actionOne = 0;    // To 'remember' than an action is in progress
int btnState = 0;     // To get two states from a momentary button

// Define which pins to use for different actions
int wmTriggerPin = D8;
int btnOnePin = D5;    // Pin for first button

// Uncomment next line to use built in LED on NodeMCU/Wemos/ESP8266 (which is D4)
#define LED_BUILTIN D4

// Flag for saving data
bool shouldSaveConfig = false;


/*
 * Standard setup function
 */
void setup() {
  // Setup Arduino IDE serial monitor for "debugging"
  Serial.begin(115200);

  // ------------------------------------------------------------
  // Pin settings
  
  // Define build-in LED pin as output
  pinMode(LED_BUILTIN, OUTPUT);

  // Set button pin
  pinMode(btnOnePin, INPUT);

  // Set trigger pin
  pinMode(wmTriggerPin, INPUT_PULLUP);

  // ------------------------------------------------------------
  // Get configuration from file (if it exists)
  // TBD

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

  // Unique name for this client
  clientID = strcat("MMRC ",cfgDeviceId);

  // ------------------------------------------------------------
  // Connect to MQTT broker and define function to handle callbacks
  delay(2000);    // Wait for WifiManager to start network connection
  int mqttPort = atoi(cfgMqttPort);
  client.setServer(cfgMqttServer, mqttPort);
  client.setCallback(mqttCallback);

}

/*
 *  Callback to notifying of the need to save configuration
 */
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}


/**
 * (Re)connects to MQTT broker and subscribes to one or more topics
 */
void mqttConnect() {
  char tmpTopic[254];
//  char tmpID[clientID.length()];
  
  // Convert String to char* for the client.subribe() function to work
//  clientID.toCharArray(tmpID, clientID.length()+1);

  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("MQTT connection...");
    // Attempt to connect
    if (client.connect(clientID)) {
      Serial.println("connected");
      Serial.print("MQTT client id: ");
      Serial.println(clientID);

      // Subscribe to all topics
      Serial.println("Subscribing to:");
      for (int i=0; i < nbrSubTopics; i++){
        // Convert String to char* for the client.subribe() function to work
        subTopic[i].toCharArray(tmpTopic, subTopic[i].length()+1);
  
        // ... print topic
        Serial.print(" - ");
        Serial.println(tmpTopic);

        //   ... and subscribe to topic
        client.subscribe(tmpTopic);
      }
    } else {
       // Count number of connection tries
      mqttRetry += 1;

      // More than 5 tries, start configuration portal
      if (mqttRetry > 5){
       Serial.println("failed, starting config portal");
        mqttRetry = 0;
        configPortal();
      } else {
       Serial.print("failed no=");
       Serial.print(mqttRetry);
       Serial.print(", rc=");
       Serial.print(client.state());
       Serial.println(", try again in 5 seconds");
       // Wait 5 seconds before retrying
       delay(5000);
      }
    }
  }
  Serial.println("---");

}

/**
 * Function to handle subscriptions
 */
//void mqttSubscribe {
//}

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
      digitalWrite(LED_BUILTIN, HIGH);
      mqttPublish(pubTopicOne, "0");
    }
    if (sbPayload == "1") {
      // Turn LED off and don't report back (via MQTT)
      digitalWrite(LED_BUILTIN, LOW);
      mqttPublish(pubTopicOne, "1");
    }
  }

}


/**
 * Publish messages to MQTT broker 
 */
void mqttPublish(String pbTopic, String pbPayload) {

  // Convert String to char* for the client.publish() function to work
  char msg[pbPayload.length()+1];
  pbPayload.toCharArray(msg, pbPayload.length()+1);
  char tpc[pbTopic.length()+1];
  pbTopic.toCharArray(tpc, pbTopic.length()+1);

  // Report back to pubTopic[]
  int check = client.publish(tpc, msg);

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
 * Show WifiManager configuration portal
 */
void configPortal() {

  // WiFiManagerlocal intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;

  // Reset settings - for testing only!
  //wifiManager.resetSettings();

  // Sets timeout until configuration portal gets turned off
  //wifiManager.setTimeout(300);

  // Make additional MQTT configuration
  WiFiManagerParameter wm_mqtt_head("<br><strong>MQTT configuration</strong>");
  WiFiManagerParameter wm_mqtt_server("brokerIp", "MQTT broker IP", cfgMqttServer, 20);
  WiFiManagerParameter wm_mqtt_port("brokerPort", "MQTT port no", cfgMqttPort, 6);
  WiFiManagerParameter wm_mqtt_retry("brokerRetry", "MQTT retry no", cfgMqttRetry, 6);
  wifiManager.addParameter(&wm_mqtt_head);
  wifiManager.addParameter(&wm_mqtt_server);
  wifiManager.addParameter(&wm_mqtt_port);
  wifiManager.addParameter(&wm_mqtt_retry);

  // Make additional Device configuration
  WiFiManagerParameter wm_device_head("<br><br><strong>Device configuration</strong>");
  WiFiManagerParameter wm_device_id("deviceId", "Device ID", cfgDeviceId, 40);
  WiFiManagerParameter wm_device_name("deviceName", "Device name", cfgDeviceName, 40);
  wifiManager.addParameter(&wm_device_head);
  wifiManager.addParameter(&wm_device_id);
  wifiManager.addParameter(&wm_device_name);

  // Make additional Node 01 configuration
  WiFiManagerParameter wm_node01_head("<br><br><strong>Node configuration</strong>");
  WiFiManagerParameter wm_node01_name("node01Name", "Node name", cfgNode01Name, 40);
  WiFiManagerParameter wm_node01_prop01_name("node01Prop01Name", "First property name", cfgNode01Prop01Name, 40);
  WiFiManagerParameter wm_node01_prop02_name("node01Prop02Name", "Second property name", cfgNode01Prop02Name, 40);
  wifiManager.addParameter(&wm_node01_head);
  wifiManager.addParameter(&wm_node01_name);
  wifiManager.addParameter(&wm_node01_prop01_name);
  wifiManager.addParameter(&wm_node01_prop02_name);

  // Make other additional configuration
  WiFiManagerParameter wm_special_head("<br><br><strong>Other configuration</strong>");
  WiFiManagerParameter wm_special_01("special01", "Special number 1", cfgSpecial01, 5);
  WiFiManagerParameter wm_special_02("special02", "Special number 2", cfgSpecial02, 5);
  wifiManager.addParameter(&wm_special_head);
  wifiManager.addParameter(&wm_special_01);
  wifiManager.addParameter(&wm_special_02);

  // WITHOUT THIS THE AP DOES NOT SEEM TO WORK PROPERLY WITH SDK 1.5 , update to at least 1.5.1
  //WiFi.mode(WIFI_STA);

   // Start AccessPoint "clientID" with configuration portal
  if (!wifiManager.startConfigPortal(clientID, "1234")) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);

    // Reset and try again
    ESP.reset();
    delay(5000);
  }

  // If you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");

  // Read updated configuration
  strcpy(cfgMqttServer, wm_mqtt_server.getValue());
  strcpy(cfgMqttPort, wm_mqtt_port.getValue());
  strcpy(cfgMqttRetry, wm_mqtt_retry.getValue());
  // TDB

  // Save the configuration to FS
  if (shouldSaveConfig) {
  // TBD  
  }

}


void loop()
{
  // Check connection to the MQTT broker. If no connection, try to reconnect
  if (!client.connected()) {
    mqttConnect();
    }

  // Wait for incoming messages
  client.loop();


  // Check if we want to start the WifiManager configuration portal
  if (digitalRead(wmTriggerPin) == HIGH) {
    configPortal();
  }


  // Check for button press
  int btnOnePress = digitalRead(btnOnePin);
  if (btnOnePress == LOW && actionOne == 0) {
    Serial.println("Button One pressed");
    delay(300);

    // Publish button press
    btnState = 1-btnState;    // Change state of Action
    if (btnState == 0) {
        mqttPublish(pubTopicOne, "1");
      } else {
        mqttPublish(pubTopicOne, "0");
      }
    
    // Action 1 is executing, new actions forbidden
    actionOne = 1;

    }

}
