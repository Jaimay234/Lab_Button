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
  #define BUTTON_PIN 25
  #define RED_LED_PIN 26
  #define GREEN_LED_PIN 27
  SemaphoreHandle_t  btnSemaphore;
  unsigned long buttonTime = 0;
  static int httpResponseCode = 0;
  static String payload = "{}";
  static bool isOpen = 0;
  volatile bool btn_pressed = false;
  JsonDocument doc;
  static bool testSent = false;
  // Functions

  String httpGETRequest(const char* url) {
    HTTPClient http;

    http.begin(url);
    http.setTimeout(3000);
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
    http.setTimeout(3000);
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
    if(isOpen) {
      //digital write green
      Serial.println("Setting Green");
      digitalWrite(RED_LED_PIN, LOW);

      digitalWrite(GREEN_LED_PIN, HIGH);
    }
    else{
      //digital write red
      Serial.println("Setting Red");
      digitalWrite(GREEN_LED_PIN, LOW);

      digitalWrite(RED_LED_PIN, HIGH);
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
    Serial.println("1. PIN TEST");
    pinMode(RED_LED_PIN, OUTPUT);
    pinMode(GREEN_LED_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    Serial.println("2. Post Pin test");
    //delay(500);

    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonPress, FALLING);
    Serial.println("Interrupt check");
    xTaskCreate(apiCheck, "Periodic API Polling", 10000, NULL, 1,&ApiTaskHandle);
    Serial.println("Check xtask");
  }



  void loop() {
    if (!testSent){
    httpGETRequest(serverName);
    httpPOSTRequest();
    testSent = true;
    }

    
    if (btn_pressed) {
      btn_pressed = false;
      httpGETRequest(serverName);
      httpPOSTRequest();
    }
  }
