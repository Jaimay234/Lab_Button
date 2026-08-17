#include <WiFi.h>
#include <HTTPClient.h> 
//libraries above are built into the esp32
#include <ArduinoJson.h> //external so download on arduino ide
#include "secrets.h"


// Variables 

#define timeDelay 5000
#define BUTTON_PIN 4
#define RED_LED_PIN 10
#define GREEN_LED_PIN 11
unsigned long buttonTime = 0;
static int httpResponseCode = 0;
static String payload = "{}";
static bool isOpen = 0;
unsigned long lastButtonPress = 0;
volatile bool btn_pressed = false;
JsonDocument doc;

// Functions

String httpGETRequest(const char* url){
  HTTPClient http;

  http.begin(url);
  http.addHeader("Authorization", String("Bearer ") + API_KEY);

  
  httpResponseCode = http.GET();
  
  if (httpResponseCode>0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
    deserializeJson(doc, payload);

    isOpen = doc["isOpen"];
  }

  else{
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }

  http.end();

  return payload;
}

void httpPOSTRequest() {
  HTTPClient http;

  http.begin(serverName);

  http.addHeader("Authorization", String("Bearer ") + API_KEY);
  http.addHeader("Content-Type", "application/json");

  doc["isOpen"] = !(isOpen);
  doc["updatedBy"] = "Lab Button";
  doc["message"] = "The lab button was pressed, Lab status updated";
  String requestBody;
  serializeJson(doc, requestBody);

  httpResponseCode = http.POST(requestBody);

  if (httpResponseCode > 0 ) {
    payload = http.getString();
    Serial.println(payload);
  }
  http.end();
}

void apiCheck() {
  httpGETRequest(serverName);
  if(isOpen){
    //digitalwrite to pin green
  }
  else{
    //digital write to led pin red
  }
}


void ARDUINO_ISR_ATTR buttonPress(){
  btn_pressed = true;
}

//test for pushing

void setup() {
  Serial.begin(115200);
  WiFi.begin(INTERNET, INTERNET_PASSWORD);
  Serial.print("Connecting");
  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED){
    delay(300);
    Serial.print("Connected to the internet");
  }
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  delay(500);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN),buttonPress, FALLING);
}



void loop() {
  unsigned long currentTime = millis();
  if (currentTime - lastButtonPress >= 100 && btn_pressed == true){
    lastButtonPress = currentTime;
    btn_pressed = false;
    httpGETRequest(serverName);
    httpPOSTRequest();
  }
  if (currentTime - buttonTime >= timeDelay){
    buttonTime = currentTime;
    apiCheck();
  }

}
