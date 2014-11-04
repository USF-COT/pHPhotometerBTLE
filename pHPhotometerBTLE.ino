#include "Photometer.h"

#include <SPI.h>
#include "Adafruit_BLE_UART.h"
#include "BTLE.h"
#include "ASConductivity.h"

#include "OneWire.h"
#include "OneWireTemperature.h"

#include <math.h>

#define BLUELEDPIN 9
#define GREENLEDPIN 8
#define DETECTORPIN A1

#define DEBUG

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

// Conductivity Class Setup
OneWireTemperature tempProbe(A0);
float tempReadFun(){
  float temperature = tempProbe.getTemperature();
  if(temperature < 0){
    Serial.print(F("Error retrieving temperature.  Received: ")); Serial.println(temperature);
    return 25;
  }
  return temperature;
}
ASConductivity condProbe(tempReadFun);

//BTLE Handlers and Utilities
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

// HACK: Adafruit lib chokes on strings over 20 chars
//       must send chunks of 20 chars at a time.
void sendBTLEString(char* sendBuffer, unsigned int length, Adafruit_BLE_UART* uart){
  unsigned int bytesRemaining;
  
  for(unsigned int i = 0; i < length; i+=20){
    bytesRemaining = min(length - i, 20);
    uart->write((uint8_t*)sendBuffer + i, bytesRemaining);
  }
  
  #ifdef DEBUG
  sendBuffer[length] = '\0';
  Serial.print("BTLE Sent: "); Serial.print(sendBuffer);
  #endif
}

void sendBlank(volatile uint8_t* buffer, volatile uint8_t len, Adafruit_BLE_UART* uart){
  unsigned int length = 0;
  unsigned int bytesRemaining;
  
  char sendBuffer[128];
  sendBuffer[length++] = 'C';
  
  // Get blank
  photometer.takeBlank();
  PHOTOREADING blank;
  photometer.getBlank(&blank);
  
  // Get conductivity
  CONDREADING condReading;
  condProbe.getReading(&condReading);
  
  sendBuffer[length++] = ',';
  length += floatToTrimmedString(sendBuffer + length, condReading.conductivity);
  sendBuffer[length++] = ',';
  length += floatToTrimmedString(sendBuffer + length, condReading.temperature);
  sendBuffer[length++] = ',';
  length += floatToTrimmedString(sendBuffer + length, condReading.salinity);
  sendBuffer[length++] = ',';
  length += floatToTrimmedString(sendBuffer + length, blank.green);
  sendBuffer[length++] = ',';
  length += floatToTrimmedString(sendBuffer + length, blank.blue);
  
  sendBuffer[length++] = '\r';
  sendBuffer[length++] = '\n';
  
  sendBTLEString(sendBuffer, length, uart);
}

void sendData(volatile uint8_t* buffer, volatile uint8_t len, Adafruit_BLE_UART* uart){
  unsigned int length = 0;
  unsigned int bytesRemaining;
  
  char sendBuffer[256];
  sendBuffer[length++] = 'D';
  
  // Get photometer reading
  photometer.takeSample();
  PHOTOREADING blankReading;
  PHOTOREADING sampleReading;
  ABSREADING absReading;
  photometer.getBlank(&blankReading);
  photometer.getSample(&sampleReading);
  photometer.getAbsorbance(&absReading);
  
  // Get conductivity
  CONDREADING condReading;
  condProbe.getReading(&condReading);
  
  // TODO: Calculate pH
  float T = condReading.temperature + 273.15;
  float pH = (-246.64209+0.315971*condReading.salinity);
  pH += (0.00028855*condReading.salinity*condReading.salinity);
  pH += (7229.23864-7.098137*condReading.salinity-0.057034*condReading.salinity*condReading.salinity)/T;
  pH += (44.493382-0.052711*condReading.salinity)*log(T);
  pH -= (0.0781344*T);
  pH += log10(((absReading.R-(-0.007762+0.000045174*T))/(1-(absReading.R*(-0.020813+0.000260262*T+0.00010436*(condReading.salinity-35))))));
  
  sendBuffer[length++] = ',';
  length += floatToTrimmedString(sendBuffer + length, condReading.conductivity);
  sendBuffer[length++] = ',';
  length += floatToTrimmedString(sendBuffer + length, condReading.temperature);
  sendBuffer[length++] = ',';
  length += floatToTrimmedString(sendBuffer + length, condReading.salinity);
  sendBuffer[length++] = ',';
  length += floatToTrimmedString(sendBuffer + length, pH);
  sendBuffer[length++] = ',';
  length += floatToTrimmedString(sendBuffer + length, blankReading.green);
  sendBuffer[length++] = ',';
  length += floatToTrimmedString(sendBuffer + length, blankReading.blue);
  sendBuffer[length++] = ',';
  length += floatToTrimmedString(sendBuffer + length, sampleReading.green);
  sendBuffer[length++] = ',';
  length += floatToTrimmedString(sendBuffer + length, sampleReading.blue);
  sendBuffer[length++] = ',';
  length += floatToTrimmedString(sendBuffer + length, absReading.Abs1);
  sendBuffer[length++] = ',';
  length += floatToTrimmedString(sendBuffer + length, absReading.Abs2);
  sendBuffer[length++] = ',';
  length += floatToTrimmedString(sendBuffer + length, absReading.R);
  
  sendBuffer[length++] = '\r';
  sendBuffer[length++] = '\n';
  
  sendBTLEString(sendBuffer, length, uart);
}

void setupBTLEHandlers(){
  addBTLERXHandler(new HandlerItem('B', sendBlank));
  addBTLERXHandler(new HandlerItem('R', sendData));
}

// Arduino Main Functions
Adafruit_BLE_UART* btle;
void setup(){
  Serial.begin(38400);
  while(!Serial);
  
  // Initialize hardware SS pin
  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);
  
  Serial.print(F("Photometer Setup..."));
  setupPhotometer();
  Serial.println(F("OK"));
  
  Serial.print(F("Conductivity Probe Setup..."));
  condProbe.begin(&Serial3);
  Serial.println(F("OK"));
  
  Serial.print(F("BTLE Hardware Setup..."));
  btle = setupBTLE("pH-1");
  Serial.println(F("OK"));
  
  Serial.print(F("BTLE Handlers Setup..."));
  setupBTLEHandlers();
  Serial.println(F("OK"));
}

void loop(){
  btle->pollACI();
}
