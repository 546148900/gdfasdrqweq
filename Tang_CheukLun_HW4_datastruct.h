#ifndef TANG_CHEUKLUN_HW4_DATASTRUCT_H
#define TANG_CHEUKLUN_HW4_DATASTRUCT_H



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>




typedef struct DoubleLinkedList
{
    struct DoubleLinkedList *prevNode;
    
    struct DoubleLinkedList *nextNode;

    int data;

    char* nameRecord;

    int *dispatchReceivedArr;
    int drSize, drCap;
    int minDR, maxDR;
    double sumDR;

    int *onsceneEnrouteArr;
    int oeSize, oeCap;
    int minOE, maxOE;
    double sumOE;

    int *onsceneReceivedArr;
    int orSize, orCap;
    int minOR, maxOR;
    double sumOR;
    
   
}DoubleLinkedList;



typedef struct {

    struct DoubleLinkedList *headNode;
    
    struct DoubleLinkedList *tailNode;

}LinkedList;



LinkedList* createList(void);

void insertAtTail(LinkedList* list, const char*name ,int width);

void releaseLinkedList(LinkedList *list);

int calulateLength(LinkedList* list);

void insertOrUpdateEventStats(LinkedList *list, const char *eventType,
                              int diffDispatchReceived,
                              int diffOnsceneEnroute,
                              int diffOnsceneReceived);




void printEventStatsPage(LinkedList *list, int page, const char *title);


#endif