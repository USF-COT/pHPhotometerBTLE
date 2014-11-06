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
  this->serial->print(F("C,0\r"));
  this->serial->print(F("RESPONSE,0\r"));
}

void ASConductivity::getReading(CONDREADING* dst){
  // Try to set temperature adjustment first
  dst->temperature = this->tempReadFun();
  if(dst->temperature >= 0){
    this->serial->print(F("T,")); this->serial->print(dst->temperature); this->serial->print(F("\r"));
  }
  
  // Request a single reading
  this->serial->print(F("R\r"));
  byte bytesRead = this->serial->readBytesUntil('\r', this->buffer, ASBUFMAX);
  if(bytesRead > 0){
    this->buffer[bytesRead] = '\0';
    Serial.print(F("Conductivity (")); Serial.print(bytesRead); Serial.print(F("): "));  Serial.print(this->buffer);
    char* conductivity = strtok(this->buffer, ",");
    dst->conductivity = atof(conductivity);
    char* salinity = strtok(NULL, "\r");
    dst->salinity = atof(salinity);
  }
}

void ASConductivity::sendCommand(const char* command, char* response){
  this->serial->print(command); this->serial->print('\r');
  byte bytesRead = this->serial->readBytesUntil('\r', response, ASBUFMAX);
  response[bytesRead] = '\0';
}
