#include "ASConductivity.h"
#include <string.h>

ASConductivity::ASConductivity(TEMPREADFUN _tempReadFun){
  this->tempReadFun = _tempReadFun;
  this->serial = NULL;
}

ASConductivity::~ASConductivity(){ 
}

void ASConductivity::begin(HardwareSerial* _serial){
  this->serial = _serial;
  this->serial->begin(38400);
  this->serial->setTimeout(2000);
  this->sendCommand("RESPONSE,1", NULL, 0);
  this->sendCommand("C,0", NULL, 0);
}

void ASConductivity::getReading(CONDREADING* dst){
  // Try to set temperature adjustment first
  dst->temperature = this->tempReadFun();
  if(dst->temperature >= 0){
    this->serial->print(F("T,")); this->serial->print(dst->temperature); this->serial->print(F("\r"));
    this->receiveResponseCode();
  }
  
  // Request a single reading
  char responseBuffer[32];
  byte bytesRead = this->sendCommand("R", responseBuffer, 32);
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

void ASConductivity::flushReceive(){
  while(this->serial->available()){
    this->serial->read();
  }
}

byte ASConductivity::receiveResponseCode(){
  char responseCode[4];
  byte bytesRead = this->serial->readBytesUntil('\r', responseCode, 4);
  responseCode[3] = '\0';
  
  Serial.print(F("AS Conductivity Response: ")); Serial.println(responseCode);
  return bytesRead;
}

byte ASConductivity::receiveResponse(char* response, byte responseBufferLength){
  byte bytesRead = 0;
  if(response){
    bytesRead = this->serial->readBytesUntil('\r', response, responseBufferLength);
    response[min(bytesRead, responseBufferLength-1)] = '\0';
  }
  
  return bytesRead;
}

byte ASConductivity::sendCommand(const char* command, char* response, byte responseBufferLength){
  this->flushReceive();
  this->serial->print(command); this->serial->print(F("\r"));
  
  // Check if this is a command that sends a response first
  byte bytesRead = 0;
  if(command[0] == 'R' && strlen(command) == 1){
    Serial.print(F("Received Read Request.  Command: "));  Serial.println(command);
    bytesRead = this->receiveResponse(response, responseBufferLength);
  } else if (command[0] == 'X' ||
             strcmp(command, "SLEEP") == 0 ){
    this->receiveResponseCode();
    bytesRead = this->receiveResponse(response, responseBufferLength);
  } else {
    bytesRead = this->receiveResponse(response, responseBufferLength);
    this->receiveResponseCode();
  }
  
  return bytesRead;
}
