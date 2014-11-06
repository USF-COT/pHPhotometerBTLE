#include "CommandQueue.h"

QueueItem::QueueItem(BTLERXHandler* _handler):handler(_handler){
  this->next = NULL;
}

void QueueItem::setNext(QueueItem* item){
  this->next = item;
}

CommandQueue::CommandQueue(){
  this->head = NULL;
}

void CommandQueue::push(BTLERXHandler* handler){
  QueueItem* newItem = new QueueItem(handler);
  QueueItem* current = this->head;
  while(current->getNext() != NULL){
    current = current->getNext();
  }
  current->setNext(newItem);
}

void CommandQueue::runNext(){
  // TODO: Implement runner
}
