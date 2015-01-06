#include "ASConductivity.h"
#include <string.h>
#include <Wire.h>
#include "utilities.h"

ASConductivity::ASConductivity(TEMPREADFUN _tempReadFun){
  this->tempReadFun = _tempReadFun;
}

ASConductivity::~ASConductivity(){ 
}

void ASConductivity::begin(byte SCLpin, byte SDApin){
  Wire.beginOnPins(SCLpin, SDApin);
  this->sendCommand("RESPONSE,1", NULL, 0);
  this->sendCommand("C,0", NULL, 0);
}

void ASConductivity::getReading(CONDREADING* dst){
  char responseBuffer[48];
  
  // Try to set temperature adjustment first
  char tempCommand[16];
  dst->temperature = this->tempReadFun();
  if(dst->temperature >= 0 && dst->temperature < 85){
    tempCommand[0] = 'T';
    tempCommand[1] = ',';
    unsigned int len = floatToTrimmedString(tempCommand + 2, dst->temperature);
    tempCommand[len++] = '\r';
    tempCommand[len++] = 0;
    this->sendCommand(tempCommand, NULL, 0);
  }
  
  // Request a single reading
  byte bytesRead = this->sendCommand("R", responseBuffer, 48);
  if(bytesRead > 0){
    Serial.print(F("Conductivity (")); Serial.print(bytesRead); Serial.print(F("): "));  Serial.println(responseBuffer);
    char* conductivity = strtok(responseBuffer, ",");
    dst->conductivity = atof(conductivity);
    char* salinity = strtok(NULL, "\r");
    dst->salinity = atof(salinity);
  } else {
    Serial.println(F("No data received from conductivity board"));
  }
}

byte ASConductivity::receiveResponse(char* response, byte responseBufferLength){
  Wire.requestFrom(ASCONADDRESS, responseBufferLength+1, 1);  // +1 for code
  byte code = Wire.read();
  
  switch (code){
    case 1:
      Serial.println(F("Success"));
      break;
    case 2:
      Serial.println(F("Failed"));
      break;
    case 254:
      Serial.println(F("Pending"));
      return code;
    case 255:
      Serial.println(F("No Data"));
      return code;
  }
  
  char in_char;
  uint8_t i;
  while(Wire.available() && i < responseBufferLength){
    in_char = Wire.read();
    response[i++] = in_char;
    if(in_char == 0){
      Wire.endTransmission();
      break;
    }
  }
  return code;
}

byte ASConductivity::sendCommand(const char* command, char* response, byte responseBufferLength){
  Serial.print(F("Sending to AS Cond: ")); Serial.print(command);
  
  Wire.beginTransmission(ASCONADDRESS);
  Wire.write(command);
  Wire.endTransmission();
  
  // Delay per requirements in Atlas Scientific Docs
  // https://www.atlas-scientific.com/_files/code/ec-i2c.pdf?
  if(command[0] == 'R' || command[0] == 'C'){
    delay(1400);
  } else {
    delay(300);
  }
  
  byte code = this->receiveResponse(response, responseBufferLength);
  
  // Delay and try again if pending
  if(code == 254){
    Serial.print(F("Retrying after 500ms..."));
    delay(500);
    this->receiveResponse(response, responseBufferLength);
  }
  return code;
}
