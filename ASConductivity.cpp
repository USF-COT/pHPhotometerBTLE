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
  this->serial->print("C,0\r");
  this->serial->readBytesUntil('\r', this->buffer, ASBUFMAX);  // Remove OK from serial buffer
}

void ASConductivity::getReading(CONDREADING* dst){
  // Try to set temperature adjustment first
  dst->temperature = this->tempReadFun();
  if(dst->temperature >= 0){
    this->serial->print(F("T,")); this->serial->print(dst->temperature); this->serial->print(F("\r"));
    this->serial->readBytesUntil('\r', this->buffer, ASBUFMAX);  // Remove OK from serial buffer
  }
  
  // Request a single reading
  this->serial->print(F("R\r"));
  byte bytesRead = this->serial->readBytesUntil('\r', this->buffer, ASBUFMAX);
  if(bytesRead > 0){
    this->buffer[bytesRead] = '\0';
    char* conductivity = strtok(this->buffer, ",");
    dst->conductivity = atof(conductivity);
    char* salinity = strtok(NULL, "\r");
    dst->salinity = atof(salinity);
  }
}
