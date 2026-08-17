#include <WiFi.h>
#include <HTTPClient.h>
#include <mutex>
//libraries above are built into the esp32
#include <ArduinoJson.h>  //external so download on arduino ide
#include "secrets.h"


//Declaration of task handle
TaskHandle_t ApiTaskHandle = NULL;

// Variables

#define timeDelay 5000
#define BUTTON_PIN 4
#define RED_LED_PIN 10
#define GREEN_LED_PIN 11
SemaphoreHandle_t  btnSemaphore;
unsigned long buttonTime = 0;
static int httpResponseCode = 0;
static String payload = "{}";
static bool isOpen = 0;
volatile bool btn_pressed = false;
JsonDocument doc;

// Functions

String httpGETRequest(const char* url) {
  HTTPClient http;

  http.begin(url);
  http.addHeader("Authorization", String("Bearer ") + API_KEY);

  xSemaphoreTake(btnSemaphore, portMAX_DELAY);
  httpResponseCode = http.GET();

  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
    deserializeJson(doc, payload);

    isOpen = doc["isOpen"];
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  xSemaphoreGive(btnSemaphore);
  http.end();
  return payload;
}

void httpPOSTRequest() {
  HTTPClient http;

  http.begin(serverName);

  http.addHeader("Authorization", String("Bearer ") + API_KEY);
  http.addHeader("Content-Type", "application/json");
  xSemaphoreTake(btnSemaphore, portMAX_DELAY);
  doc.clear();
  doc["isOpen"] = !(isOpen);
  doc["updatedBy"] = "Lab Button";
  doc["message"] = "The lab button was pressed, Lab status updated";
  String requestBody;
  serializeJson(doc, requestBody);
  httpResponseCode = http.POST(requestBody);
  if (httpResponseCode > 0) {
    payload = http.getString();
    Serial.println(payload);
  }
  xSemaphoreGive(btnSemaphore);
  http.end();
}

void apiCheck(void *parameter) {
  while(true){
  delay(1000);
  httpGETRequest(serverName);
  if (isOpen) {
    //digitalwrite to pin green
  } else {
    //digital write to led pin red
  }
}
}


void ARDUINO_ISR_ATTR buttonPress() {
  unsigned long currentTime = millis();
  if (currentTime - buttonTime >= 100) {
    btn_pressed = true;
    buttonTime = currentTime;
  } 
}

void pollRequest() {
  httpGETRequest(serverName);
  delay(100);
}

void setup() {
  Serial.begin(115200);
  btnSemaphore = xSemaphoreCreateMutex();
  WiFi.begin(INTERNET, INTERNET_PASSWORD);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    delay(300);
    Serial.print("Connected to the internet");
  }
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  delay(500);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonPress, FALLING);

  xTaskCreatePinnedToCore(apiCheck, "Periodic API Polling", 10000, NULL, 1,&ApiTaskHandle,1);

}



void loop() {

  if (btn_pressed) {
    btn_pressed = false;
    httpGETRequest(serverName);
    httpPOSTRequest();
  }
}
