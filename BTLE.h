#ifndef BTLEFUNCTIONS_H
#define BTLEFUNCTIONS_H

#include <Arduino.h>
#include <SPI.h>
#include "Adafruit_BLE_UART.h"

#define ADAFRUITBLE_REQ 34
#define ADAFRUITBLE_RDY 3
#define ADAFRUITBLE_RST 36

Adafruit_BLE_UART* setupBTLE(char* broadcastName);

#define RXBUFFERMAX 256
static volatile uint8_t rxBuffer[RXBUFFERMAX];
static volatile uint8_t rxLength = 0;

typedef void (*BTLERXHandler)(volatile uint8_t*, volatile uint8_t, Adafruit_BLE_UART*);
struct HANDLERITEM{
  char prefix;
  BTLERXHandler handler;
  HANDLERITEM* next;
};

class HandlerItem{
  private:
    char prefix;
    BTLERXHandler handler;
    HandlerItem* next;
  public:
    HandlerItem(char prefix, BTLERXHandler);
    ~HandlerItem();
    
    boolean isPrefix(char prefix);
    void setNext(HandlerItem* item);
    HandlerItem* getNext();
    void runHandler(uint8_t* buffer, uint8_t len, Adafruit_BLE_UART* uart);
    
};

void addBTLERXHandler(HandlerItem* newItem);

#endif
