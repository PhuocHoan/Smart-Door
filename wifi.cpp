#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

#define SSID "Wokwi-GUEST"
#define PASSWORD ""
// Defining the WiFi channel speeds up the connection:
#define WIFI_CHANNEL 6

//***Set server***
const char* mqttServer = "broker.hivemq.com"; 
int port = 1883;

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

void wifiConnect() {
  WiFi.begin(SSID, PASSWORD, WIFI_CHANNEL);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print("."); 
  }
  Serial.println(" Connected!");
}

void mqttConnect() {
  while(!mqttClient.connected()) {
    Serial.println("Attemping MQTT connection...");
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    if(mqttClient.connect(clientId.c_str())) {
      Serial.println("connected");

      //***Subscribe all topic you need***
      mqttClient.subscribe("/22127119/led");
     
    }
    else {
      Serial.println("try again in 5 seconds");
      delay(5000);
    }
  }
}

//MQTT Receiver
void callback(char* topic, byte* message, unsigned int length) {
    Serial.println(topic);
    String strMsg;
    for(int i=0; i<length; i++) {
        strMsg += (char)message[i];
    }
    Serial.println(strMsg);

    //***Code here to process the received package***
    if (strMsg == "on") {
        digitalWrite(15, HIGH);
    }
    else {
        digitalWrite(15, LOW);
    }
}

void setup() {
    Serial.begin(9600);
    Serial.print("Connecting to WiFi");

    wifiConnect();
    
    /* -----------------Tạm thời comment----------------- */ 
    // mqttClient.setServer(mqttServer, port);
    // mqttClient.setCallback(callback);
    // mqttClient.setKeepAlive( 90 );
}


void loop() {
    /* -----------------Tạm thời comment----------------- */ 
    // if(!mqttClient.connected()) {
    // mqttConnect();
    // }
    // mqttClient.loop();

    // //***Publish data to MQTT Server***
    // int temp = random(0,100);
    // char buffer[50];
    // sprintf(buffer, "%d", temp);
    // mqttClient.publish("/22127119/temp", buffer);


    // delay(5000);


}