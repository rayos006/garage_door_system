#include <ESP8266WiFi.h> 
#include <PubSubClient.h>
#include "ArduinoJson.h"
#include "globals.h"

//GPIO
const int button = 12;
void ICACHE_RAM_ATTR door_changed ();
//Millis
unsigned long PREV_TIME = 0;
const long DURATION = 5000;
const long PERIOD = 10000;

// Door state
//True = Open, False = Closed
bool door_state = false;

//Wifi
WiFiClient espClient;

//MQTT
PubSubClient client(espClient);
// Runs when pin changes states
void door_changed(){

}
  
void setup() {
  //Serial Port begin
  Serial.begin (9600);

  //Define inputs and outputs

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

  //Door state change
  pinMode(button, INPUT_PULLUP);
  attachInterrupt(button, door_changed, CHANGE);
  
}

void connect_mqtt() {
  // Loop until reconnected
  while (!client.connected()) {
    Serial.println("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266 Client", mqtt_username, mqtt_password)) {
      Serial.println("Connected");
      } 
    else {
      Serial.println("failed, rc=");
      Serial.print(client.state());
      delay(5000);
    }
  }
}

void loop() {
  unsigned long curr_time = millis();
  if (!client.connected()) {
    connect_mqtt();
      // Send Config to Home Assistant
      DynamicJsonDocument doc(512);
      doc["name"] = "Garage Door2";
      doc["device_class"] = "garage_door";
      doc["state_topic"] = "homeassistant/binary_sensor/garage/door_sensor/state";
      char buffer[512];
      size_t n = serializeJson(doc, buffer);
      client.publish(hassio_config, buffer, n);
  }
  client.loop();
  if(curr_time - PREV_TIME >= PERIOD) {
    if(door_state){
      client.publish(hassio_state, "ON");
    }
    else {
      client.publish(hassio_state, "OFF");
  }
  PREV_TIME = curr_time;
  }
}
