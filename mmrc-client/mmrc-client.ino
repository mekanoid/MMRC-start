#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "MMRCsettings.h"

WiFiClient wifiClient;
PubSubClient client(wifiClient);

// Convert settings in MMRCsettings.h to constants and variables
const char* ssid = SSID;
const char* password = PASSWORD;
const char* mqttBrokerIP = IP;
String cccCategory = CATEGORY;
String cccModule = MODULE;
String cccObject = OBJECT;
String cccCLEES = CLEES;

// Variable for topics to subscribe to
const int nbrTopics = 1;
String subTopic[nbrTopics];

// Variable for topics to publish to
String pubTopic[1];

// Variables for client info
String clientID;


/*
 * Standard setup function
 */
void setup() {
  // Setup Arduino IDE serial monitor for "debugging"
  Serial.begin(115200);

  // Define build-in LED pin as output
  pinMode(LED_BUILTIN, OUTPUT);

  // Assemble topics to subscribe and publish to
  if (cccCLEES == "1") {
    cccCategory = "clees";
    subTopic[0] = cccCategory+"/"+cccModule+"/cmd/turnout/"+cccObject;
    pubTopic[0] = cccCategory+"/"+cccModule+"/rpt/turnout"+cccObject;
  } else {
    subTopic[0] = "mmrc/"+cccModule+"/"+cccObject+"/"+cccCategory+"/turnout/cmd";
    subTopic[1] = "mmrc/"+cccModule+"/"+cccObject+"/"+cccCategory+"/button/cmd";
    pubTopic[0] = "mmrc/"+cccModule+"/"+cccObject+"/"+cccCategory+"/turnout/rpt";
 }

  // Unique name for this client
  if (cccCLEES == "1") {
    clientID = "CLEES "+cccObject;
  } else {
    clientID = "MMRC "+cccObject;
  }

  // Connect to wifi network
  wifiConnect();
  delay(1000);

  // Connect to MQTT broker and define function to handle callbacks
  client.setServer(mqttBrokerIP, 1883);
  client.setCallback(mqttCallback);

}


/**
 * Connects to WiFi and prints out connection information
 */
void wifiConnect() {
  char tmpID[clientID.length()];
  delay(200);

   // Convert String to char* for the client.subribe() function to work
  clientID.toCharArray(tmpID, clientID.length()+1);

  // Connect to WiFi
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.print(ssid);
  WiFi.hostname(tmpID);
  WiFi.begin(ssid, password);
 
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
 
  // Print connection information
  Serial.print("Client hostname: ");
  Serial.println(WiFi.hostname());
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("MAC address: ");
  Serial.println(WiFi.macAddress());
  Serial.println("---");
}


/**
 * (Re)connects to MQTT broker and subscribes to one or more topics
 */
void mqttConnect() {
  char tmpTopic[254];
  char tmpID[clientID.length()];
  
  // Convert String to char* for the client.subribe() function to work
  clientID.toCharArray(tmpID, clientID.length()+1);

  // Loop until we're reconnected
  while (!client.connected()) {
  Serial.print("MQTT connection...");
  // Attempt to connect
  if (client.connect(tmpID)) {
    Serial.println("connected");
    Serial.print("MQTT client id: ");
    Serial.println(tmpID);
    Serial.println("Subscribing to:");

    // Subscribe to all topics
    for (int i=0; i < nbrTopics; i++){
      // Convert String to char* for the client.subribe() function to work
      subTopic[i].toCharArray(tmpTopic, subTopic[i].length()+1);

      // ... print topic
      Serial.print(" - ");
      Serial.println(tmpTopic);

      // ... and subscribe to topic
      client.subscribe(tmpTopic);
    }
  } else {
    Serial.print("failed, rc=");
    Serial.print(client.state());
    Serial.println(" try again in 5 seconds");
    // Wait 5 seconds before retrying
    delay(5000);
    }
  }
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
      digitalWrite(LED_BUILTIN, HIGH);
      mqttPublish(pubTopic[0], "0");
    }
    if (sbPayload == "1") {
      // Turn LED off and don't report back (via MQTT)
      digitalWrite(LED_BUILTIN, LOW);
      mqttPublish(pubTopic[0], "1");
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

  // Print information
  Serial.print("Message sent    [");
  Serial.print(pbTopic);
  Serial.print("] ");
  Serial.println(pbPayload);

}


void loop()
{
  // Check connection to the MQTT broker. If no connection, try to reconnect
  if (!client.connected()) {
    mqttConnect();
  }

  // Wait for incoming messages
  client.loop();
}
