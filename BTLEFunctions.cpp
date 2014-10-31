#include "BTLEFunctions.h"
#include <stdlib.h>

Adafruit_BLE_UART uart(ADAFRUITBLE_REQ, ADAFRUITBLE_RDY, ADAFRUITBLE_RST);

/**************************************************************************/
/*!
    This function is called whenever select ACI events happen
*/
/**************************************************************************/
void aciCallback(aci_evt_opcode_t event)
{
  switch(event)
  {
    case ACI_EVT_DEVICE_STARTED:
      Serial.println(F("Advertising started"));
      break;
    case ACI_EVT_CONNECTED:
      Serial.println(F("Connected!"));
      break;
    case ACI_EVT_DISCONNECTED:
      Serial.println(F("Disconnected or advertising timed out"));
      break;
    default:
      break;
  }
}

HandlerItem* HandlerHead = NULL;
/**************************************************************************/
/*!
    This function is called whenever data arrives on the RX channel
*/
/**************************************************************************/
void rxCallback(uint8_t *buffer, uint8_t len)
{
  // Fill receive buffer
  if((rxLength + len) > RXBUFFERMAX){
    len = RXBUFFERMAX - (rxLength + len);
  }
  strncpy((char *)(rxBuffer + rxLength), (char*)buffer, len);
  rxLength += len;
  
  for(int i=0; i < rxLength; ++i){
    Serial.write(rxBuffer[i]);
  }
  Serial.println();
  Serial.print("Length: "); Serial.println(rxLength);
  
  // If line end detected, process the data.
  if(rxBuffer[rxLength - 1] == '\n'){
    Serial.print(F("Received: ")); Serial.print((char*) rxBuffer);
    HandlerItem* possibleHandler = HandlerHead;
    while(possibleHandler != NULL){
      if(possibleHandler->isPrefix(rxBuffer[0])){
        possibleHandler->runHandler((uint8_t*) rxBuffer, rxLength, &uart);
        break;
      }
    }
    
    if(possibleHandler == NULL){
      Serial.print(F("No handler found for character: ")); Serial.println(rxBuffer[0]);
    }
    
    rxLength = 0;
    rxBuffer[rxLength] = '\0';
  }
}

void setupBTLE(){
  uart.setRXcallback(rxCallback);
  uart.setACIcallback(aciCallback);
  uart.begin();
  uart.setDeviceName("pH-1");
}

HandlerItem::HandlerItem(char _prefix, BTLERXHandler _handler):prefix(_prefix), handler(_handler){
  this->next = NULL;
}

HandlerItem::~HandlerItem(){}

boolean HandlerItem::isPrefix(char prefix){
  return this->prefix == prefix;
}

void HandlerItem::setNext(HandlerItem* item){
  this->next = item;
}

HandlerItem* HandlerItem::getNext(){
  return this->next;
}

void HandlerItem::runHandler(uint8_t* buffer, uint8_t len, Adafruit_BLE_UART* uart){
  this->handler(buffer, len, uart);
}

void addBTLERXHandler(HandlerItem* newItem){
  if(HandlerHead == NULL){
    // Set this as the head of the handler list
    HandlerHead = newItem;
  } else {
    // Find last element in singly linked list
    HandlerItem* currentItem = HandlerHead;
    HandlerItem* nextItem = currentItem->getNext();
    while(nextItem != NULL){
      currentItem = nextItem;
      nextItem = currentItem->getNext();
    }
    currentItem->setNext(newItem);
  }
}
