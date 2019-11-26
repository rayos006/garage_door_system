#include <ESP8266WiFi.h> 
#include <PubSubClient.h>
#include "ArduinoJson.h"
#include "globals.h"

//GPIO
const int button = 12;
const int relay = 5;
void ICACHE_RAM_ATTR door_changed ();

//Wifi
WiFiClient espClient;

//MQTT
PubSubClient client(espClient);
// Runs when pin changes states
void door_changed(){
    Serial.println(digitalRead(relay));
    digitalWrite(relay, HIGH);
    delay(5000);
    digitalWrite(relay, LOW);
}

//Callback for the garage relay
void callback(char *topic, byte *payload, unsigned int length) {
  static char message[MAX_MSG_LEN+1];
  if (length > MAX_MSG_LEN) {
    length = MAX_MSG_LEN;
  }
  strncpy(message, (char *)payload, length);
  message[length] = '\0';
  Serial.println(topic);
  Serial.println(message);
  Serial.println(length);

  if(strcmp(message, "ON") == 0){
    digitalWrite(relay, HIGH);
    delay(5000);
    digitalWrite(relay, LOW);
  }
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
  client.setCallback(callback);

  //Door state change
  pinMode(button, INPUT_PULLUP);
  pinMode(relay, OUTPUT);
  attachInterrupt(button, door_changed, CHANGE);
  
}

void connect_mqtt() {
  // Loop until reconnected
  while (!client.connected()) {
    Serial.println("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266 Relay", mqtt_username, mqtt_password)) {
      Serial.println("Connected");
      client.subscribe(on_off_topic);
      } 
    else {
      Serial.println("failed, rc=");
      Serial.print(client.state());
      delay(5000);
    }
  }
}

void loop() {
  DynamicJsonDocument doc(512);
  if (!client.connected()) {
      connect_mqtt();
        // Send Config to Home Assistant
        char buffer[512];
        size_t n = serializeJson(doc, buffer);
        client.publish(config_topic, buffer, n);
        doc["name"] = "Garage Relay";
        //doc["state_topic"] = "homeassistant/binary_sensor/garage/relay/state";
        client.publish("homeassistant/binary_sensor/garage/relay/config", buffer, n);
    }
  client.loop();
}
