#ifndef ASCONDUCTIVITY_H
#define ASCONDUCTIVITY_H

#include "Arduino.h"

struct CONDREADING{
  float conductivity;
  float temperature;
  float salinity;
};

typedef float (*TEMPREADFUN)();

#define ASBUFMAX 30

class ASConductivity{
  private:
    HardwareSerial* serial;
    char buffer[ASBUFMAX];
    
    TEMPREADFUN tempReadFun;
  public:
    ASConductivity(TEMPREADFUN);
    ~ASConductivity();
    
    void begin(HardwareSerial* _serial);
    void getReading(CONDREADING* dst);
};

#endif
