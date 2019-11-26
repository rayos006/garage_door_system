 #include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "ArduinoJson.h"
#include "globals.h"

//Stoplight
bool flash_status = false;

//Millis
unsigned long PREV_TIME = 0;
const long PERIOD = 500;
const long LONG_PERIOD = 1000;

//Wifi
WiFiClient espClient;

//MQTT
PubSubClient client(espClient);

void callback(char *topic, byte *payload, unsigned int length) {
  static char message[MAX_MSG_LEN+1];
  if (length > MAX_MSG_LEN) {
    length = MAX_MSG_LEN;
  }
  strncpy(message, (char *)payload, length);
  message[length] = '\0';
  if(strcmp(message, "OFF") == 0){
    client.publish(state_topic, "OFF");
    flash_status = false;
    off();
  }
  else if(strcmp(message, "green") == 0){
    client.publish(state_topic, "ON");
    flash_status = false;
    green();
  }
  else if(strcmp(message, "yellow") == 0){
    client.publish(state_topic, "ON");
    flash_status = false;
     yellow();
  }
  else if(strcmp(message, "red") == 0){
    client.publish(state_topic, "ON");
    flash_status = false;
    red();
  }
  else {
    client.publish(state_topic, "ON");
    flash_status = true;
  }
}

void setup() {
  //Serial Port begin
  Serial.begin (9600);

  //Define inputs and outputs
  pinMode(D1, OUTPUT);
  pinMode(D2, OUTPUT);
  pinMode(D3, OUTPUT);

  // WIFI Info and connect
  WiFi.begin(ssid, wifi_password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.println("Waiting to connect…");
  }

  // IP Addresses
  Serial.print("MY IP address: ");
  Serial.println(WiFi.localIP());

  //MQTT PUB/SUB
  client.setServer(mqtt_hostname, 1883);
  client.setCallback(callback);

}

void connect_mqtt() {
  DynamicJsonDocument doc(512);
  // Loop until reconnected
  while (!client.connected()) {
    Serial.println("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266 Stoplight", mqtt_username, mqtt_password)) {
      client.subscribe(light_topic);
      Serial.println("Connected");
      // Send Config to Home Assistant
      doc["name"] = "Garage Light";
      //doc["state_topic"] = "homeassistant/binary_sensor/garage/distance_sensor/state";
      char buffer[512];
      size_t n = serializeJson(doc, buffer);
      client.publish(config_topic, buffer, n);
      } 
    else {
      Serial.println("failed, rc=");
      Serial.print(client.state());
      delay(5000);
    }
  }
}

void loop() {
  // Current time
  unsigned long curr_time = millis();
  // put your main code here, to run repeatedly:
  if (!client.connected()) {
    connect_mqtt();
  }
  client.loop();
  if(flash_status){
    if (curr_time - PREV_TIME >= PERIOD) {
      red();
      if (curr_time - PREV_TIME >= LONG_PERIOD) {
        off();
        PREV_TIME = curr_time;
      }
    }
  }
}

void off(){
  digitalWrite(D2, false);
  digitalWrite(D3, false);
  digitalWrite(D1, false);
  Serial.println("ALL OFF");
}

// Turns on the red LED
void red(){
  digitalWrite(D2, false);
  digitalWrite(D3, false);
  digitalWrite(D1, true);
  Serial.println("RED ON");
}

// Turns on the yellow LED
void yellow(){
  digitalWrite(D1, false);
  digitalWrite(D3, false);
  digitalWrite(D2, true);
  Serial.println("YELLOW ON");
}

// Turns on the green LED
void green(){
  digitalWrite(D1, false);
  digitalWrite(D2, false);
  digitalWrite(D3, true);
  Serial.println("GREEN ON");
}
