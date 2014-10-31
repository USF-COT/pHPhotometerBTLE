/* Photometer - A library for controlling LEDs and light voltage converters
 * to sample solution characteristics.
 *
 * By: Michael Lindemuth
 * University of South Florida
 * College of Marine Science
 */
 
#ifndef PHOTOMETER_H
#define PHOTOMETER_H

typedef void (* PinControlFunPtr) (int);
typedef int (* DetectorReadFunPtr) ();

struct PHOTOREADING{
  float blue;
  float green;
};

struct ABSREADING{
  float Abs1;
  float Abs2;
  float R;
};

class Photometer{
  private:
    PinControlFunPtr blueLightControl, greenLightControl;
    DetectorReadFunPtr detectorRead;
    
    PHOTOREADING blank, sample;
    ABSREADING absReading;
  
  public:
    Photometer(PinControlFunPtr blueLightControl, PinControlFunPtr greenLightControl, DetectorReadFunPtr detectorRead);
    ~Photometer();
    
    void takeBlank();
    void takeSample();
    
    void setBlank(PHOTOREADING* src);
    
    void resetBlank();
    void getBlank(PHOTOREADING* dest);
    
    void resetSample();
    void getSample(PHOTOREADING* dest);
    void getAbsorbance(ABSREADING* dest);
};
    
#endif
