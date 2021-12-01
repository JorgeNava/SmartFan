#include "Arduino.h"
#include <EMailSender.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include <DHT.h>
#include <DHT_U.h>
#define DHTTYPE DHT11
#define DHT11_PIN 0
DHT dht(DHT11_PIN, DHTTYPE);

const char* ssid = "Test";
const char* password = "hola1234";
const char* origin_email = "jorgenavadelapena@gmail.com";
const char* origin_password = "navajita";
const char* destiny_email = "account_to_send@gmail.com";
const int RELAY_PIN = 2;
String RELAY_STATE;
boolean relay_state_updated = false;
uint8_t connection_state = 0;
uint16_t reconnect_interval = 10000;

EMailSender emailSend(origin_email, origin_password);

AsyncWebServer server(80);

String getTemperature() {
 float temperature = dht.readTemperature();
 Serial.println(temperature);
 return String(temperature);
}

String getHumidity() {
 float humidity = dht.readHumidity();
 Serial.println(humidity);
 return String(humidity);
}

String processor(const String& var){
 Serial.println(var);
 if(var == "STATE"){
   if(digitalRead(RELAY_PIN)){
    RELAY_STATE = "ON";
   }
   else{
    RELAY_STATE = "OFF";
   }
   Serial.print(RELAY_STATE);
   return RELAY_STATE;
 }else if (var == "TEMPERATURE"){
   return getTemperature();
 }else if (var == "HUMIDITY"){
   return getHumidity();
 }
}
void setup(){
 dht.begin();
 Serial.begin(9600);
 pinMode(RELAY_PIN, OUTPUT);

 if(!SPIFFS.begin()){
 Serial.println("ha ocurrido un error al montar SPIFFS");
 return;
 }
 WiFi.begin(ssid, password);
 while (WiFi.status() != WL_CONNECTED) {
 delay(1000);
 Serial.println("Connecting to WiFi..");
 }

 Serial.println();
 Serial.println(WiFi.SSID());
 Serial.print("IP address:\t");
 Serial.println(WiFi.localIP());

 server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
 request->send(SPIFFS, "/index.html", String(), false, processor);
 });

 server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
 request->send(SPIFFS, "/style.css", "text/css");
 });

 server.on("/on", HTTP_GET, [](AsyncWebServerRequest *request){
 digitalWrite(RELAY_PIN, HIGH);
 relay_state_updated = true;
 request->send(SPIFFS, "/index.html", String(), false, processor);
 });

 server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request){
 digitalWrite(RELAY_PIN, LOW);
 relay_state_updated = true;
 request->send(SPIFFS, "/index.html", String(), false, processor);
 });
 server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
 request->send_P(200, "text/plain", getTemperature().c_str());
 });

 server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
 request->send_P(200, "text/plain", getHumidity().c_str());
 });

 server.begin();
}
void loop(){
 float temperature = dht.readTemperature();
 Serial.println("Temperature: ");
 Serial.println(temperature);
 if(temperature > 18){
    RELAY_STATE = "ON";
    relay_state_updated = true;
    digitalWrite(RELAY_PIN, HIGH);
 }else{
    RELAY_STATE = "OFF";
    relay_state_updated = true;
    digitalWrite(RELAY_PIN, LOW);
 }

 if(relay_state_updated){
    EMailSender::EMailMessage message;
    message.subject = "Vent state updated.";
    message.message = "Hello beatiful user! Your vent state has been updated to: " + RELAY_STATE;

    EMailSender::Response resp = emailSend.send(destiny_email, message);

    Serial.println("Sending status: ");

    Serial.println(resp.status);
    Serial.println(resp.code);
    Serial.println(resp.desc);
     relay_state_updated = false;
 }
}
