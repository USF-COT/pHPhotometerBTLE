#ifndef ASCONDUCTIVITY_H
#define ASCONDUCTIVITY_H

#include "Arduino.h"
#define ASCONADDRESS 100

struct CONDREADING{
  float conductivity;
  float temperature;
  float salinity;
};

typedef float (*TEMPREADFUN)();

class ASConductivity{
  private:
    TEMPREADFUN tempReadFun;
    
    void flushReceive();
    byte receiveResponseCode();
    byte receiveResponse(char* response, byte responseBufferLength);
  public:
    ASConductivity(TEMPREADFUN);
    ~ASConductivity();
    
    void begin(byte SCLpin, byte SDApin);
    void getReading(CONDREADING* dst);
    byte sendCommand(const char* command, char* response, byte responseBufferLength);
    byte sendCommandCodeFlipped(const char* command, char* response, byte responseBufferLength);
};

#endif
