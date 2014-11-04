#ifndef TEMPERATURE_H
#define TEMPERATURE_H

#include "Arduino.h"
#include "OneWire.h"

class OneWireTemperature{
  private:
    OneWire ds;
    byte data[12];
    byte addr[8];
  public:
    OneWireTemperature(int pin);
    ~OneWireTemperature();
    
    float getTemperature();
};

#endif
