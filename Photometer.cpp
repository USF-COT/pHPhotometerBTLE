#include "Photometer.h"
#include <Arduino.h>
#include <math.h>

Photometer::Photometer(PinControlFunPtr blueLightControl, PinControlFunPtr greenLightControl, DetectorReadFunPtr detectorRead){
  this->blueLightControl = blueLightControl;
  this->greenLightControl = greenLightControl;
  this->detectorRead = detectorRead;
  
  this->sample.blue = this->sample.green = 0;
  this->blank.blue = this->blank.green = 0;
}

Photometer::~Photometer(){
}

float averageSample(PinControlFunPtr lightControl, DetectorReadFunPtr detectorRead){
  const float NUM_SAMPLES = 10;
  const int LIGHT_DELAY = 2000;
  
  lightControl(HIGH);
  delay(LIGHT_DELAY);
  
  int sample = 0;
  for(unsigned short i=0; i < NUM_SAMPLES; ++i){
    sample += detectorRead();
  }
  lightControl(LOW);
  
  return sample /= NUM_SAMPLES;
}

void Photometer::takeBlank(){  
  this->blank.blue = averageSample(this->blueLightControl, this->detectorRead);
  this->blank.green = averageSample(this->greenLightControl, this->detectorRead);
}

void Photometer::takeSample(){  
  this->sample.blue = averageSample(this->blueLightControl, this->detectorRead);
  this->sample.green = averageSample(this->greenLightControl, this->detectorRead);
  
  this->absReading.Abs1 = log((float)this->blank.blue/(float)this->sample.blue)/(log(10));//calculate the absorbance
  this->absReading.Abs2 = log((float)this->blank.green/(float)this->sample.green)/(log(10));//calculate the absorbance
  if(this->absReading.Abs1 == 0){
    this->absReading.R=(float)this->absReading.Abs2/(float)this->absReading.Abs1;
}

void Photometer::resetBlank(){
  this->blank.blue = 0;
  this->blank.green = 0;
}

void Photometer::getBlank(PHOTOREADING* dest){
  dest->blue = this->blank.blue;
  dest->green = this->blank.green;
}

void Photometer::resetSample(){
  this->sample.blue = 0;
  this->sample.green = 0;
}

void Photometer::getSample(PHOTOREADING* dest){
  dest->blue = this->sample.blue;
  dest->green = this->sample.green;
}

void Photometer::getAbsorbance(ABSREADING* dest){
  dest->Abs1 = this->absReading.Abs1;
  dest->Abs2 = this->absReading.Abs2;
  dest->R = this->absReading.R;
}
