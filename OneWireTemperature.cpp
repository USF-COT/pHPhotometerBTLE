#include "OneWireTemperature.h"

OneWireTemperature::OneWireTemperature(int pin):ds(pin){
}

OneWireTemperature::~OneWireTemperature(){ 
}

float OneWireTemperature::getTemperature(){
  if ( !this->ds.search(this->addr)) {
      //no more sensors on chain, reset search
      this->ds.reset_search();
      return -1000;
  }

  if ( OneWire::crc8( this->addr, 7) != this->addr[7]) {
      Serial.println("CRC is not valid!");
      return -2000;
  }

  if ( this->addr[0] != 0x10 && this->addr[0] != 0x28) {
      return -3000;
  }

  this->ds.reset();
  this->ds.select(this->addr);
  this->ds.write(0x44,1); // start conversion, with parasite power on at the end

  byte present = this->ds.reset();
  this->ds.select(this->addr);    
  this->ds.write(0xBE); // Read Scratchpad

  
  for (int i = 0; i < 9; i++) { // we need 9 bytes
    this->data[i] = this->ds.read();
  }
  
  this->ds.reset_search();
  
  byte MSB = data[1];
  byte LSB = data[0];

  float tempRead = ((MSB << 8) | LSB); //using two's compliment
  float TemperatureSum = tempRead / 16;
  
  return TemperatureSum;
}
