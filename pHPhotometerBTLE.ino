#include "Photometer.h"

#include <SPI.h>
#include <Wire.h>
#include "utilities.h"
#include <RFduinoBLE.h>
#include "BTLE.h"
#include "ASConductivity.h"
#include "Adafruit_MCP23008.h"

#include "OneWire.h"
#include "OneWireTemperature.h"

#include <math.h>

#define BLUELEDPIN 7
#define GREENLEDPIN 0
#define DETECTORPIN 6

#define DEBUG

// I2C Wire Setup
#define SCLPIN 3
#define SDAPIN 4
void setupWire(){
  Wire.beginOnPins(SCLPIN, SDAPIN);
}

// Photometer Setup
Adafruit_MCP23008 mcp;
void blueLEDControl(int level){
  mcp.digitalWrite(BLUELEDPIN, level);
}
void greenLEDControl(int level){
  mcp.digitalWrite(GREENLEDPIN, level);
}
int readLightConverter(){
  return analogRead(DETECTORPIN);
}
Photometer photometer(blueLEDControl, greenLEDControl, readLightConverter);

void setupPhotometer(){
  mcp.begin();
  mcp.pinMode(BLUELEDPIN, OUTPUT);
  mcp.digitalWrite(BLUELEDPIN, LOW);
  mcp.pinMode(GREENLEDPIN, OUTPUT);
  mcp.digitalWrite(GREENLEDPIN, LOW);
  
  pinMode(DETECTORPIN, INPUT);
}

// Conductivity Class Setup
OneWireTemperature tempProbe(5);
float tempReadFun(){
  float temperature = tempProbe.getTemperature();
  if(temperature < 0){
    Serial.print(F("Error retrieving temperature.  Received: ")); Serial.println(temperature);
    return 25;
  }
  return temperature;
}
ASConductivity condProbe(tempReadFun);

//BTLE Handlers
static unsigned short blankReadings;
static volatile boolean sendBlankPending;
void sendBlankHandler(volatile char* buffer, volatile int len){
  if(!sendBlankPending){
    blankReadings = 0;
    sendBlankPending = true;
  }
}

#define BLANKDIFFLIMIT 4
#define BLANKREADLIMIT 5

void sendBlank(){
  unsigned int length = 0;
  unsigned int bytesRemaining;
  
  char sendBuffer[128];
  
  // Get blank
  PHOTOREADING oldBlank, currentBlank;
  photometer.getBlank(&oldBlank);
  photometer.takeBlank();
  photometer.getBlank(&currentBlank);
  
  float greenBlankDiff = abs(oldBlank.green - currentBlank.green);
  float blueBlankDiff = abs(oldBlank.blue - currentBlank.blue);
  
  if(++blankReadings > BLANKREADLIMIT){
    Serial.println(F("Blank reading limit exceeded.  Returning value without convergence."));
    greenBlankDiff = 0;
    blueBlankDiff = 0;
  }
  
  if(greenBlankDiff <= BLANKDIFFLIMIT && blueBlankDiff <= BLANKDIFFLIMIT){
    // This is a good reading, no more necessary
    sendBlankPending = false;
    
    // Get conductivity
    CONDREADING condReading;
    condProbe.getReading(&condReading);
    
    sendBuffer[length++] = 'C';
    sendBuffer[length++] = ',';
    length += dtostrf(condReading.conductivity, 2, sendBuffer + length);
    sendBuffer[length++] = ',';
    length += dtostrf(condReading.temperature, 2, sendBuffer + length);
    sendBuffer[length++] = ',';
    length += dtostrf(condReading.salinity, 2, sendBuffer + length);
    sendBuffer[length++] = ',';
    
    length += dtostrf(currentBlank.green, 2, sendBuffer + length);
    sendBuffer[length++] = ',';
    length += dtostrf(currentBlank.blue, 2, sendBuffer + length);
    
    sendBuffer[length++] = '\r';
    sendBuffer[length++] = '\n';
    
    sendBTLEString(sendBuffer, length);
  } else {
    Serial.println(F("Reading not stable."));
    Serial.print(F("Green Diff: ")); Serial.println(greenBlankDiff);
    Serial.print(F("Blue Diff: ")); Serial.println(blueBlankDiff);
  }
}

static volatile boolean sendDataPending = false;
static unsigned short dataReadings;
void sendDataHandler(volatile char* buffer, volatile int len){
  if(!sendDataPending){
    dataReadings = 0;
    sendDataPending = true;
  }
}

#define SAMPLEDIFFLIMIT 4
#define SAMPLEREADLIMIT 5

void sendData(){
  unsigned int length = 0;
  unsigned int bytesRemaining;
  
  char sendBuffer[256];
  sendBuffer[length++] = 'D';
  
  // Get photometer reading
  PHOTOREADING blankReading;
  PHOTOREADING oldSample, currentSample;
  photometer.getSample(&oldSample);
  photometer.takeSample();
  photometer.getSample(&currentSample);
  
  float greenSampleDiff = abs(oldSample.green - currentSample.green);
  float blueSampleDiff = abs(oldSample.blue - currentSample.blue);
  
  if(++dataReadings > SAMPLEREADLIMIT){
    Serial.println(F("Sample read limit exceeded.  Returning non-converged reading."));
    greenSampleDiff = 0;
    blueSampleDiff = 0;
  }
  
  if(greenSampleDiff <= SAMPLEDIFFLIMIT && blueSampleDiff <= SAMPLEDIFFLIMIT){
    // Samples have converged, do not do another sample
    sendDataPending = false;
    
    ABSREADING absReading;
    photometer.getBlank(&blankReading);
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
    length += dtostrf(condReading.conductivity, 2, sendBuffer + length);
    sendBuffer[length++] = ',';
    length += dtostrf(condReading.temperature, 2, sendBuffer + length);
    sendBuffer[length++] = ',';
    length += dtostrf(condReading.salinity, 2, sendBuffer + length);
    sendBuffer[length++] = ',';
    length += dtostrf(pH, 2, sendBuffer + length);
    sendBuffer[length++] = ',';
    length += dtostrf(blankReading.green, 2, sendBuffer + length);
    sendBuffer[length++] = ',';
    length += dtostrf(blankReading.blue, 2, sendBuffer + length);
    sendBuffer[length++] = ',';
    length += dtostrf(currentSample.green, 2, sendBuffer + length);
    sendBuffer[length++] = ',';
    length += dtostrf(currentSample.blue, 2, sendBuffer + length);
    sendBuffer[length++] = ',';
    length += dtostrf(absReading.Abs1, 2, sendBuffer + length);
    sendBuffer[length++] = ',';
    length += dtostrf(absReading.Abs2, 2, sendBuffer + length);
    sendBuffer[length++] = ',';
    length += dtostrf(absReading.R, 2, sendBuffer + length);
    
    sendBuffer[length++] = '\r';
    sendBuffer[length++] = '\n';
    
    sendBTLEString(sendBuffer, length);
  } else {
    Serial.println(F("Reading not stable."));
    Serial.print(F("Green Diff: ")); Serial.println(greenSampleDiff);
    Serial.print(F("Blue Diff: ")); Serial.println(blueSampleDiff);
    /*
    // Samples have not converged.  Send update and resample
    sendBuffer[length++] = 'U';
    sendBuffer[length++] = ',';
    
    length += dtostrf(sendBuffer + length, greenSampleDiff);
    sendBuffer[length++] = ',';
    length += dtostrf(sendBuffer + length, blueSampleDiff);
    sendBuffer[length++] = ',';
    
    length += dtostrf(sendBuffer + length, oldSample.green);
    sendBuffer[length++] = ',';
    length += dtostrf(sendBuffer + length, oldSample.blue);
    sendBuffer[length++] = ',';
    length += dtostrf(sendBuffer + length, currentSample.green);
    sendBuffer[length++] = ',';
    length += dtostrf(sendBuffer + length, currentSample.blue);
    */
  }
}

void sendTemperatureHandler(volatile char* buffer, volatile int len){
  char sendBuffer[32];
  unsigned int length = 0;
  
  sendBuffer[length++] = 'T';
  sendBuffer[length++] = ',';
  
  float temperature = tempProbe.getTemperature();
  
  length += dtostrf(temperature, 2, sendBuffer + length);
  sendBuffer[length++] = '\r';
  sendBuffer[length++] = '\n';
  
  sendBTLEString(sendBuffer, length);
}

void sendConductivityString(volatile char* buffer, volatile int len){
  char command[48];
  char response[48];
  strncpy(command, (char*)buffer+1, 48);
  
  byte code = condProbe.sendCommand(command, response, 48);
  char* errorResponse = NULL;
  switch(code){
    case ASFAILURE:
      errorResponse = "Failed to Send\r\n";
      break;
    case ASPENDING:
      errorResponse = "Pending\r\n";
      break;
    case ASNODATA:
      errorResponse = "No Data\r\n";
      break;
  }
  if(errorResponse){
    sendBTLEString(errorResponse, strlen(errorResponse));
  } else {
    sendBTLEString(response, strlen(response));
  }
}

void setupBTLEHandlers(){
  addBTLERXHandler(new HandlerItem('B', sendBlankHandler));
  addBTLERXHandler(new HandlerItem('R', sendDataHandler));
  addBTLERXHandler(new HandlerItem('T', sendTemperatureHandler));
  addBTLERXHandler(new HandlerItem('X', sendConductivityString));
}

// Arduino Main Functions
void setup(){
  Serial.begin(9600);
  while(!Serial);
  
  Serial.print(F("Reassigning I2C Pins..."));
  setupWire();
  Serial.println(F("OK"));
  
  Serial.print(F("Photometer Setup..."));
  setupPhotometer();
  Serial.println(F("OK"));
  
  Serial.print(F("BTLE Hardware Setup..."));
  setupBTLE("pH-1");
  Serial.println(F("OK"));
  
  Serial.print(F("BTLE Handlers Setup..."));
  setupBTLEHandlers();
  Serial.println(F("OK"));
}

void loop(){
  if(sendBlankPending){
    sendBlank();
  }
  
  if(sendDataPending){
    sendData();
  }
}
