#ifndef TANG_CHEUKLUN_HW4_DATASTRUCT_H
#define TANG_CHEUKLUN_HW4_DATASTRUCT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>

typedef struct DoubleLinkedList
{
    struct DoubleLinkedList *prevNode;

    struct DoubleLinkedList *nextNode;

    int data;

    char *nameRecord;

    int *list1;
    int size1;
    int maxSize1;

    int *list2;
    int size2;
    int maxSize2;

    int *list3;
    int size3;
    int maxSize3;

} DoubleLinkedList;

typedef struct LinkedList
{
    DoubleLinkedList *headNode;

    DoubleLinkedList *tailNode;

    pthread_mutex_t lock;

} LinkedList;

typedef struct
{
    const char *filename;        // 資料檔案
    LinkedList *headerList;      // Header List (欄位定義)
    LinkedList *eventStatsTotal; // 全城市
    LinkedList *eventStatsSub1;  // 子欄位1
    LinkedList *eventStatsSub2;  // 子欄位2
    const char *subfieldName;    // 例如 "police_district"
    const char *subVal1;         // 例如 "BAYVIEW"
    const char *subVal2;         // 例如 "MISSION"
    int lengthPerRecord;         // 單筆紀錄長度
    long startRecord;            // 要處理的起始筆 (含)
    long endRecord;              // 要處理的結束筆 (不含)
} ThreadArg;

void insertAtTail(LinkedList *list, const char *name, int width);

LinkedList *createList();

int calulateLength(LinkedList *list);

void insertAtTail(LinkedList *list, const char *name, int width);

void releaseLinkedList(LinkedList *list);

void insertData(LinkedList *list, const char *type, int diffDispatchReceived, int diffOnsceneEnroute, int diffOnsceneReceived);

int bubble_sort_inner(int A[], int N);

void bubble_sort(int A[], int N);

void printPage(LinkedList *list, char *title, const char *sub);

int checkEqual(const char *firstString, const char *secondString);

#endif