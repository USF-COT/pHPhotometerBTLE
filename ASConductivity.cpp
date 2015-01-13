#include "ASConductivity.h"
#include <string.h>
#include <Wire.h>
#include "utilities.h"

ASConductivity::ASConductivity(TEMPREADFUN _tempReadFun){
  this->tempReadFun = _tempReadFun;
}

ASConductivity::~ASConductivity(){ 
}

void ASConductivity::getReading(CONDREADING* dst){
  char responseBuffer[48];
  
  // Try to set temperature adjustment first
  char tempCommand[16];
  dst->temperature = this->tempReadFun();
  if(dst->temperature >= 0 && dst->temperature < 85){
    tempCommand[0] = 'T';
    tempCommand[1] = ',';
    unsigned int len = dtostrf(dst->temperature, 2, tempCommand + 2);
    tempCommand[len++] = '\0';
    this->sendCommand(tempCommand, NULL, 0);
  }
  
  // Request a single reading
  byte responseCode = this->sendCommand("R", responseBuffer, 48);
  if(responseCode == ASSUCCESS){
    Serial.print(F("Conductivity (")); Serial.print(strlen(responseBuffer)); Serial.print(F("): "));  Serial.println(responseBuffer);
    char* conductivity = strtok(responseBuffer, ",");
    dst->conductivity = atof(conductivity);
    char* salinity = strtok(NULL, ",");
    dst->salinity = atof(salinity);
  } else {
    Serial.println(F("No data received from conductivity board"));
  }
}

byte ASConductivity::receiveResponse(char* response, byte responseBufferLength){
  Wire.requestFrom(ASCONADDRESS, responseBufferLength+1, true);  // +1 for code
  byte code = Wire.read();
  
  switch (code){
    case ASSUCCESS:
      Serial.println(F("Success"));
      break;
    case ASFAILURE:
      Serial.println(F("Failed"));
    case ASPENDING:
      Serial.println(F("Pending"));
    case ASNODATA:
      Serial.println(F("No Data"));
      Wire.endTransmission();
      return code;
  }
  
  char in_char;
  uint8_t i=0;
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
  Serial.print(F("Sending to AS Cond: ")); Serial.println(command);
  
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
    code = this->receiveResponse(response, responseBufferLength);
  }
  return code;
}
