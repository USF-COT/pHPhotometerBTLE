#ifndef PHUTILITIES_H
#define PHUTILITIES_H

#include <Arduino.h>

char *dtostrf (double val, signed char width, unsigned char prec, char *sout);
unsigned int floatToTrimmedString(char* dest, float value);

#endif
