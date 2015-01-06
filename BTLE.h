#ifndef BTLEFUNCTIONS_H
#define BTLEFUNCTIONS_H

#include <Arduino.h>
#include <RFduinoBLE.h>

void setupBTLE(char* broadcastName);
void sendBTLEString(char* sendBuffer, unsigned int length);

#define RXBUFFERMAX 256
static volatile char rxBuffer[RXBUFFERMAX];
static volatile int rxLength = 0;

typedef void (*BTLERXHandler)(volatile char*, volatile int len);

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
    void runHandler(volatile char* buffer, volatile int len);
    
};

void addBTLERXHandler(HandlerItem* newItem);

#endif
