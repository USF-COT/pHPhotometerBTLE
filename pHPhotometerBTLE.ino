#include "Photometer.h"

#include <SPI.h>
#include "Adafruit_BLE_UART.h"
#include "BTLEFunctions.h"

#define BLUELEDPIN 9
#define GREENLEDPIN 8
#define DETECTORPIN A1

// Photometer Setup
void blueLEDControl(int level){
  digitalWrite(BLUELEDPIN, level);
}
void greenLEDControl(int level){
  digitalWrite(GREENLEDPIN, level);
}
int readLightConverter(){
  return analogRead(DETECTORPIN);
}
Photometer photometer(blueLEDControl, greenLEDControl, readLightConverter);

void setupPhotometer(){
  pinMode(BLUELEDPIN, OUTPUT);
  pinMode(GREENLEDPIN, 8);
  pinMode(DETECTORPIN, INPUT);
}

// Setup BTLE Handlers Code
unsigned int floatToTrimmedString(char* dest, float value){
  char floatBuffer[16];
  unsigned int floatLength, offset;
  
  dtostrf(value, 8, 3, floatBuffer);
  floatLength = strlen(floatBuffer);
  offset = 0;
  for(offset=0; offset < 12; ++offset){
    if(floatBuffer[offset] != 0x20){
      break;
    }
  }
    
  floatLength -= offset;
  strncpy(dest, floatBuffer + offset, floatLength);
  
  return floatLength;
}

void sendBlank(volatile uint8_t* buffer, volatile uint8_t len, Adafruit_BLE_UART* uart){
  unsigned int length = 0;
  unsigned int bytesRemaining;
  
  char sendBuffer[128];
  sendBuffer[length++] = 'C';
  
  float blank[5] = { 20, 30, 6, 138, 139 };
  for(unsigned int i = 0; i < 5; ++i){
    sendBuffer[length++] = ',';
    blank[i] += random(-300, 300)/(float)100;
    length += floatToTrimmedString(sendBuffer + length, blank[i]);
  }
  
  sendBuffer[length++] = '\r';
  sendBuffer[length++] = '\n';
  
  for(unsigned int i = 0; i < length; i+=20){
    bytesRemaining = min(length - i, 20);
    uart->write((uint8_t*)sendBuffer + i, bytesRemaining);
  }
}

void setupBTLEHandlers(){
  addBTLERXHandler(new HandlerItem('B', sendBlank));
}

// Arduino Main Functions
void setup(){
  Serial.begin(38400);
  while(!Serial);
  
  // Initialize hardware SS pin
  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);
  
  setupPhotometer();
  setupBTLE();
  setupBTLEHandlers();
}

void loop(){
  
}
