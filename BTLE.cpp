#include "BTLE.h"
#include <stdlib.h>

void RFduinoBLE_onAdvertisement(){
  Serial.println(F("Advertising..."));
}

void RFduinoBLE_onConnect()
{
  Serial.println(F("Connected!"));
}

void RFduinoBLE_onDisconnect(){
  Serial.println(F("Disconnected"));
}

HandlerItem* HandlerHead = NULL;
/**************************************************************************/
/*!
    This function is called whenever data arrives on the RX channel
*/
/**************************************************************************/
void RFduinoBLE_onReceive(char *buffer, int len)
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
  if(rxBuffer[rxLength - 1] == '\n' || rxBuffer[rxLength - 1] == 'n'){
    rxBuffer[rxLength - 1] = '\0';
    Serial.print(F("Received: ")); Serial.println((char*) rxBuffer);
    
    // Remove any numeric leader before command due to iOS RF String Sender
    int commandStart = 0;
    while(rxBuffer[commandStart] >= '0' || rxBuffer[commandStart] <= '9' || rxBuffer[commandStart] == ' ' || commandStart == rxLength){
      commandStart++;
    }
    
    if(commandStart == rxLength){
      Serial.println(F("Only numeric leader found.  Command will not be processed."));
      return;
    }
    
    if(commandStart > 0){
      Serial.print(F("Removed leader.  Processing command: ")); Serial.println((char*)rxBuffer + commandStart);
    }
    
    HandlerItem* possibleHandler = HandlerHead;
    while(possibleHandler != NULL){
      if(possibleHandler->isPrefix(rxBuffer[commandStart])){
        possibleHandler->runHandler(rxBuffer + commandStart, rxLength - commandStart);
        break;
      }
      possibleHandler = possibleHandler->getNext();
    }
    
    if(possibleHandler == NULL){
      Serial.print(F("No handler found for character: ")); Serial.println(rxBuffer[0]);
    }
    
    rxLength = 0;
    rxBuffer[rxLength] = '\0';
  }
}

void setupBTLE(char* broadcastName){
  RFduinoBLE.deviceName = broadcastName;
  RFduinoBLE.txPowerLevel = -8;
  RFduinoBLE.advertisementInterval = 100;
  RFduinoBLE.begin();
}

void sendBTLEString(char* sendBuffer, unsigned int length){
  unsigned int bytesRemaining;
  
  for(unsigned int i = 0; i < length; i+=20){
    bytesRemaining = min(length - i, 20);
    RFduinoBLE.send(sendBuffer + i, bytesRemaining);
  }
  
  #ifdef DEBUG
  sendBuffer[length] = '\0';
  Serial.print("BTLE Sent: "); Serial.print(sendBuffer);
  #endif
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

void HandlerItem::runHandler(volatile char* buffer, volatile int len){
  this->handler(buffer, len);
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
