#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
// ------- Connection Info for WIFI -------
const char* ssid = "Buttars";           // SSID of the wifi network
const char* password = "Pineapple"; // PSK of the wifi network
const char* server = "10.0.0.89";    // IP Address of the MQTT Broker
char* ledTopic = "/stoplight/led";          // Define a topic for MQTT
char* doorTopic = "/stoplight/door";
char* sonicTopic = "/stoplight/sensor";
String macAddr = WiFi.macAddress();       // Store arduino MAC address as a string
String host = "arduino-" + macAddr.substring(15) ;  // Create hostname for this device
// (should not match other MQTT devices)

WiFiClient wifiClient;                  // Instantiate wifi
PubSubClient mqttClient(wifiClient);    // Instantiate mqtt client

int state; // 0 = close, 1= open switch

const int trigPin = D6;
const int echoPin = D5;

float duration, distance;
int currentDist = 0;  // 0 = green, 1 = yellow, 2 = red, 3 = blinking
int previousDist = 0;

void setup() {
  Serial.begin(9600);                   // Start serial connection for debugging
  Serial.println("\nMQTT Sonic Distance Sensor"); // Serial Intro

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
  mqttClient.subscribe(sonicTopic);
  mqttClient.subscribe(ledTopic);
  Serial.println(mqttClient.state());   // Show debug info about MQTT state

  // ----- GPIO PIN Setup -----
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

}

void loop() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(10);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(50);
  digitalWrite(trigPin, LOW);
  
  duration = pulseIn(echoPin, HIGH);
  distance = (duration*.0343)/2;
  Serial.print("Distance: ");
  Serial.println(distance);
  delay(100);
  
  if (distance < 20 and distance > 10) { // RED
    currentDist = 2;
    if (previousDist != 2) {
      mqttClient.publish(sonicTopic, "stop");
    }
    previousDist = 2;
  }
  else if (distance >= 20 and distance < 40) { // YELLOW
    currentDist = 1;
    if (previousDist != 1) {
      mqttClient.publish(sonicTopic, "slow");
    }
    previousDist = 1;
  }
  else if (distance >= 40) { // GREEN
    currentDist = 0;
    if (previousDist != 0) {
      mqttClient.publish(sonicTopic, "go");
    }
    previousDist = 0;
  }
  else { // BLINK
    currentDist = 3;
    if (previousDist != 3) {
      mqttClient.publish(sonicTopic, "blink");
    }
    previousDist = 3;
  }
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
