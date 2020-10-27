#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
// ------- Connection Info for WIFI -------
const char* ssid = "Buttars";           // SSID of the wifi network
const char* password = "Pineapple"; // PSK of the wifi network
const char* server = "10.0.0.89";    // IP Address of the MQTT Broker
char* ledTopic = "/stoplight/led";          // Define a topic for MQTT
char* doorTopic = "/stoplight/door";
String macAddr = WiFi.macAddress();       // Store arduino MAC address as a string
String host = "arduino-" + macAddr.substring(15) ;  // Create hostname for this device
// (should not match other MQTT devices)

WiFiClient wifiClient;                  // Instantiate wifi
PubSubClient mqttClient(wifiClient);    // Instantiate mqtt client

// Const Variables
const int sensor = D4;
int currentDoorStatus = 0; // 1 = closed, 0 = open
int previousDoorStatus = 0;

int state; // 0 close - 1 open switch

void setup()
{
  Serial.begin(9600);                   // Start serial connection for debugging
  Serial.println("\nMQTT Door Sensor");// Serial Intro

  // ----- Connect to Wifi -----
  Serial.print("Connecting to '");      // Connect debug info
  Serial.print(ssid);                   // Show the SSID in debug info

  WiFi.hostname(host);                  // Set the client hostname for wifi
  WiFi.begin(ssid, password);           // Connect to Wifi

  while (WiFi.status() != WL_CONNECTED) { //Wait for connection
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");     // Wifi connection debug
  delay (1000);                         // Let everything settle

  // ----- Connect to MQTT -----
  Serial.println("Connecting to MQTT Broker"); // Serial debug
  mqttClient.setServer(server, 1883);   // Set up MQTT Connection Info
  mqttClient.setCallback(callback);     // Function to call when new messages are received
  mqttClient.connect(host.c_str());     // Connect to the broker with unique hostname
  mqttClient.subscribe(doorTopic);       // Subscribe to the LED topic on the broker
  Serial.println(mqttClient.state());   // Show debug info about MQTT state

  // ----- GPIO PIN Setup -----
  digitalWrite(LED_BUILTIN, 1);         // Initial state of onboard LED is off (1=Off)
  pinMode(LED_BUILTIN, OUTPUT);         // Set the onboard LED to output
  pinMode(sensor, INPUT_PULLUP);
  
}

void loop()
{
  mqttClient.loop();
  state = digitalRead(sensor);
  
  if (state == HIGH){
    currentDoorStatus = 1;
    if (currentDoorStatus == 1 and previousDoorStatus != 1) {
      digitalWrite(LED_BUILTIN, 1);
      previousDoorStatus = 1;

      mqttClient.publish(doorTopic, "open");
    }
  }
  else{
    currentDoorStatus = 0;
    if (currentDoorStatus == 0 and previousDoorStatus != 0) {
      digitalWrite(LED_BUILTIN, 0);
      previousDoorStatus = 0;

      mqttClient.publish(doorTopic, "closed");
    }
  }
  delay(200);
}

void callback(char* topicChar, byte* payload, unsigned int length) { // defined by pubSubClient
  String topic = (String)topicChar;     // Convert topic from char* to String
  String message = "";                  // Convert payload from byte* to String
  // MQTT won't send a null character to terminate the byte*, but it sends the
  // length, so we iterate thorugh the payload and copy each character
  for (int i = 0; i < length; i++) {    // Iterate through the characters in the payload array
    message += (char)payload[i];     //    and display each character
  }
  Serial.print("Message arrived [");    // Serial Debug
  Serial.print(topic);                  //    Print the topic name [in brackets]
  Serial.print("] ");                   //
  Serial.println(message);              //    Print the message
}
