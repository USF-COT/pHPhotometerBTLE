#include "Photometer.h"

#include <SPI.h>
#include <Wire.h>
#include "utilities.h"
#include <RFduinoBLE.h>
#include "BTLE.h"
#include "ASConductivity.h"

#include "OneWire.h"
#include "OneWireTemperature.h"

#include <math.h>

#define BLUELEDPIN 2
#define GREENLEDPIN 1
#define DETECTORPIN 6

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
  pinMode(GREENLEDPIN, OUTPUT);
  pinMode(DETECTORPIN, INPUT);
}

// Conductivity Class Setup
#define SCLPIN 3
#define SDAPIN 4

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
    length += floatToTrimmedString(sendBuffer + length, condReading.conductivity);
    sendBuffer[length++] = ',';
    length += floatToTrimmedString(sendBuffer + length, condReading.temperature);
    sendBuffer[length++] = ',';
    length += floatToTrimmedString(sendBuffer + length, condReading.salinity);
    sendBuffer[length++] = ',';
    
    length += floatToTrimmedString(sendBuffer + length, currentBlank.green);
    sendBuffer[length++] = ',';
    length += floatToTrimmedString(sendBuffer + length, currentBlank.blue);
    
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
    length += floatToTrimmedString(sendBuffer + length, currentSample.green);
    sendBuffer[length++] = ',';
    length += floatToTrimmedString(sendBuffer + length, currentSample.blue);
    sendBuffer[length++] = ',';
    length += floatToTrimmedString(sendBuffer + length, absReading.Abs1);
    sendBuffer[length++] = ',';
    length += floatToTrimmedString(sendBuffer + length, absReading.Abs2);
    sendBuffer[length++] = ',';
    length += floatToTrimmedString(sendBuffer + length, absReading.R);
    
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
    
    length += floatToTrimmedString(sendBuffer + length, greenSampleDiff);
    sendBuffer[length++] = ',';
    length += floatToTrimmedString(sendBuffer + length, blueSampleDiff);
    sendBuffer[length++] = ',';
    
    length += floatToTrimmedString(sendBuffer + length, oldSample.green);
    sendBuffer[length++] = ',';
    length += floatToTrimmedString(sendBuffer + length, oldSample.blue);
    sendBuffer[length++] = ',';
    length += floatToTrimmedString(sendBuffer + length, currentSample.green);
    sendBuffer[length++] = ',';
    length += floatToTrimmedString(sendBuffer + length, currentSample.blue);
    */
  }
}

void sendTemperatureHandler(volatile char* buffer, volatile int len){
  char sendBuffer[32];
  unsigned int length = 0;
  
  sendBuffer[length++] = 'T';
  sendBuffer[length++] = ',';
  
  float temperature = tempProbe.getTemperature();
  
  length += floatToTrimmedString(sendBuffer + length, temperature);
  sendBuffer[length++] = '\r';
  sendBuffer[length++] = '\n';
  
  sendBTLEString(sendBuffer, length);
}

void sendConductivityString(volatile char* buffer, volatile int len){
  char command[64];
  strncpy(command, (char*)buffer+1, 64);
  char response[30];
  condProbe.sendCommand(command, response, 30);
  sendBTLEString(response, strlen(response));
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
  
  // Initialize hardware SS pin
  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);
  
  Serial.print(F("Photometer Setup..."));
  setupPhotometer();
  Serial.println(F("OK"));
  
  Serial.print(F("Conductivity Probe Setup..."));
  condProbe.begin(SCLPIN, SDAPIN);
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
