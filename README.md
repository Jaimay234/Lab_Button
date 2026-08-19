# IoT Lab Status Controller

### What is it:
This is an embedded IoT device that is built around an ESP32 that allows a physical button to remotely update and monitor the status of a laboratory thorugh a REST API.

The project connects the physical computer hardware to a cloud based backend, demonstrating embedded programming, wireless communication, REST APIs, JSON serialization, interrupt handling and FreeRTOS task synchonization.

## Features:
- ESP32 based controller
- Wi-Fi connectivity
- REST API communication over HTTP
- JSON-based communication using ArduinoJson
- Physical push-button input
- GPIO interrupt handling
- Software button debouncing
- FreeRTOS task for periodic API polling
- FreeRTOS mutex for protecting HTTP requests and shared resources
- Remote state synchonization
- LED status indication
- API authentication using a bearer token
- Privacy and protection of credentials using a separate header file (secrets.h)

## Hardware used:
- ESP32 microcontroller
- Push button
- Red LED
- Green LED
- 220Ω Resistors (prevent LEDs from  burning out)
- Breadboard and wires

### Pin Configuration:

| Component | GPIO |
| --- | --- |
| Button | GPIO4 |
| Red LED | GPIO 10 |
| Green LED | GPIO 11 |

> Pin assignments may change according to needs of project or board being used.

## Software

### Firmware
The firmware was written in C++ using the Arduino framework.

Main technologies:
- C++
- ESP32
- FreeRTOS
- Wi-Fi
- HTTP/REST
- JSON
- GPIO interrupts

### Libraries
- Wifi.h
- HTTPClient.h
- ArduinoJson.h

Wifi.h and HTTPClient.h are provided by the ESP32 Ardunio framework. ArduinoJson is an external library. 

## How it works
### 1. Device Initialization
Startup:
- Initializes serial interface
- Creates FreeRTOS mutex
- Connects to the Wi-Fi network
- Configures the button and LEDs as GPIO devices
- Registers a GPIO interrupt for the button
- Starts polling the API

### 2. Button Press
Button is configured using an internal pull-up resistor.

When the button is pressed, the GPIO transitions from HIGH to LOW and triggers the interrupt. This interrupt records that the button was pressed and modifies the variable below:
volatile bool btn_pressed = false;

Instead of performing the HTTP request itself. This is then processed inside of the main loop which then triggers the function and communicates with the backend. This keeps the interrupt short and avoids performing network operations inside of an ISR. This design decision also prevents doing long operations that can block other functions and use a lot of system resources.


### 3. Updating the backend

When a button press is detected, the ESP332 retrieves the current server state and sends a POST request containing the new state.

Example payload:

```C++ 
{
  "isOpen": true,
  "updatedBy": "Lab Button",
  "message": "The lab button was pressed, Lab status updated"
} 
```

The backend becomes the authoritative source of the lab's current state.

### 4. Periodic State Synchronization

A dedicated FreeRTOS task periodically performs a GET request:

``` C++
void apiCheck(void *parameter)
```
The ESP32 retrieves the current state from the server and updates its local isOpen variable.

This allows the physical device to remain synchronized even when the state is changed via an external factor.

### 5. LED Feedback

The LEDs provide a physical representation of the lab status on the server either being:

OPEN = -> Green LED
Closed -> Red LED

This means the ESP32 is not simply sending commands to the server; it also acts as an IoT edge device that consumes data from the backend.

## Concurrency

The project uses FreeRTOS synchronization primitives to prevent multiple operations from accessing the HTTP Client/shared state simultaneously.

A mutex is created during the initialization:
``` C++
btnSemaphore = xSemaphoreCreateMutex();
```

HTTP operations acquire the mutex before accessing shared resources:
```C++
xSemaphoreTake(btnSemaphore, portMAX_DELAY);

// HTTP operations

xSemaphoreGive(btnSemaphore);
```

This is important as there are two operations fighting for the same resource and performing network operations being:
- Main loop
- periodic FreeRTOS API task

performing network operations.

## Security
Important credentials are not saved in the main file and instead in a separate header file:
secrets.h

The firmware uses the credentials to authenticated using the bearer token:
Authorization: Bearer <API_KEY>

secrets.h should not be committed to the repository.

## What was learned
This project was built to provide a simpler way to update whether a room was in use or not for a club, while also exploring the architecture of a small connected embedded system rather than controlling GPIO peripherals.

It provided practical experience with:

- Embedded C++
- ESP32 development
- GPIO interrupts
- FreeRTOS
- Synchronization and concurrency
- Wi-Fi networking
- REST APIs
- JSON serialization/deserialization
- Authentication
- Hardware/software integration
- Device to cloud communication


   
