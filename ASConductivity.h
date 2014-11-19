#ifndef ASCONDUCTIVITY_H
#define ASCONDUCTIVITY_H

#include "Arduino.h"

struct CONDREADING{
  float conductivity;
  float temperature;
  float salinity;
};

typedef float (*TEMPREADFUN)();

class ASConductivity{
  private:
    HardwareSerial* serial;
    
    TEMPREADFUN tempReadFun;
    
    void flushReceive();
    byte receiveResponseCode();
    byte receiveResponse(char* response, byte responseBufferLength);
  public:
    ASConductivity(TEMPREADFUN);
    ~ASConductivity();
    
    void begin(HardwareSerial* _serial);
    void getReading(CONDREADING* dst);
    byte sendCommand(const char* command, char* response, byte responseBufferLength);
    byte sendCommandCodeFlipped(const char* command, char* response, byte responseBufferLength);
};

#endif
