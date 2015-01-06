#include "utilities.h"

// RFDuino stdlib does not have dtostrf
char *dtostrf (double val, signed char width, unsigned char prec, char *sout) {
  char fmt[20];
  sprintf(fmt, "%%%d.%df", width, prec);
  sprintf(sout, fmt, val);
  return sout;
}

unsigned int floatToTrimmedString(char* dest, float value){
  char floatBuffer[16];
  unsigned int floatLength, offset;
  
  dtostrf(value, 8, 3, floatBuffer);
  floatLength = strlen(floatBuffer);
  offset = 0;
  for(offset=0; offset < 12; ++offset){
    if(floatBuffer[offset] != 0x20){
      break;
    }
  }
    
  floatLength -= offset;
  strncpy(dest, floatBuffer + offset, floatLength);
  
  return floatLength;
}
