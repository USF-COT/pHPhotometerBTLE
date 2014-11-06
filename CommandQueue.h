#ifndef COMMANDQUEUE_H
#define COMMANDQUEUE_H

#include <Arduino.h>
#include "BTLE.h"

class QueueItem{
  private:
    uint8_t command[32];
    uint8_t length;
    
    BTLERXHandler* handler;
    QueueItem* next;
    
  public:
    QueueItem(BTLERXHandler* _handler);
    
    QueueItem* getNext();
    void setNext(QueueItem* item);
};

class CommandQueue{
  private:
    QueueItem* head;
    
  public:
    CommandQueue();
    
    void push(BTLERXHandler* handler);
    void runNext();
};

#endif
