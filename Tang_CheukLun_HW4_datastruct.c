#include <stdio.h>
#include "Tang_CheukLun_HW4_datastruct.h"
#include <string.h>
#include <math.h>




LinkedList* createList(){
    LinkedList* nodePtr = (LinkedList*)malloc(sizeof(LinkedList));
    if(nodePtr == NULL){
        printf ("The Node is empty");
        return NULL;

    }
    nodePtr->headNode = NULL;
    nodePtr->tailNode = NULL;
    return nodePtr;

}

int calulateLength(LinkedList* list){
    int total = 0;
    
    DoubleLinkedList* currentNode = list->headNode;
    
    while (currentNode != NULL) {
        
        total += currentNode->data;
        currentNode = currentNode->nextNode;
    
    }
    return total;

}



void insertAtTail(LinkedList* list, const char*name ,int width) {
    DoubleLinkedList *newNode = (DoubleLinkedList*)malloc(sizeof(DoubleLinkedList));
    if (!newNode) {
        printf ("The Node is empty");
        return;
    }
    
    newNode->nameRecord = malloc(strlen(name) + 1); 
    if (!newNode->nameRecord) {
        printf("Memory allocation for nameRecord failed\n");
        free(newNode);
        return;
    }

    strcpy(newNode->nameRecord, name);
    
    newNode->data = width;
    newNode->nextNode = NULL;
   
    if (list->headNode == NULL) {
        newNode->prevNode = NULL;
        list->headNode = newNode;
        list->tailNode = newNode;
    } else {
        newNode->prevNode = list->tailNode;
        list->tailNode->nextNode = newNode;
        list->tailNode = newNode;
    }
}


void releaseLinkedList(LinkedList *list) { 
    if (!list) return;
    
    DoubleLinkedList *currentNode = list->headNode;
    while (currentNode) {
        DoubleLinkedList *nextNode = currentNode->nextNode;
        free(currentNode->nameRecord);
        free(currentNode);
        currentNode = nextNode;
    }

    free(list);
}


// 搜尋是否已存在該事件節點；若不存在則建立一個新節點
DoubleLinkedList* findOrCreateEventNode(LinkedList *list, const char *eventType) {
    DoubleLinkedList *current = list->headNode;
    while (current != NULL) {
        if (strcmp(current->nameRecord, eventType) == 0) {
            return current;
        }
        current = current->nextNode;
    }
    // 若找不到，則新增一個節點並初始化其統計欄位
    insertAtTail(list, eventType, 0);
    current = list->tailNode;
    current->dispatchReceivedArr = NULL;
    current->drSize = 0; 
    current->drCap = 0;
    current->onsceneEnrouteArr = NULL;
    current->oeSize = 0; 
    current->oeCap = 0;
    current->onsceneReceivedArr = NULL;
    current->orSize = 0; 
    current->orCap = 0;
    return current;
}

// 用於動態擴展整數陣列，並將新值加入陣列中
void addValueToDynamicArray(int **array, int *currentSize, int *currentCap, int value) {
    if (*currentCap == 0) {
        *currentCap = 10;
        *array = (int *)malloc(sizeof(int) * (*currentCap));
    } else if (*currentSize >= *currentCap) {
        *currentCap *= 2;
        *array = (int *)realloc(*array, sizeof(int) * (*currentCap));
    }
    (*array)[(*currentSize)++] = value;
}

// 更新指定節點的統計資料
void updateEventNodeStats(DoubleLinkedList *node, 
                          int diffDispatchReceived, 
                          int diffOnsceneEnroute, 
                          int diffOnsceneReceived) {
    node->data++;  // 統計計數器加一
    addValueToDynamicArray(&(node->dispatchReceivedArr), &(node->drSize), &(node->drCap), diffDispatchReceived);
    addValueToDynamicArray(&(node->onsceneEnrouteArr), &(node->oeSize), &(node->oeCap), diffOnsceneEnroute);
    addValueToDynamicArray(&(node->onsceneReceivedArr), &(node->orSize), &(node->orCap), diffOnsceneReceived);
}

// 將事件統計資訊插入或更新到 LinkedList 中
void insertOrUpdateEventStats(LinkedList *list, const char *eventType,
                              int diffDispatchReceived, int diffOnsceneEnroute, int diffOnsceneReceived) {
    if (!list || !eventType)
        return;
    
    DoubleLinkedList *node = findOrCreateEventNode(list, eventType);
    updateEventNodeStats(node, diffDispatchReceived, diffOnsceneEnroute, diffOnsceneReceived);
}


int compareInt(const void *a, const void *b) {
    return (*(int *)a - *(int *)b);
}

// 計算最小值與最大值
void computeMinMax(int *data, int size, int *min, int *max) {
    if (size <= 0) {
        *min = *max = 0;
        return;
    }
    *min = data[0];
    *max = data[size - 1];
}

// 計算平均值
double computeAverage(int *data, int size) {
    if (size <= 0){
        return 0.0;
    }
    
    double sum = 0;
    
    for (int i = 0; i < size; i++) {
        sum += data[i];
    }
    
    return sum / size;
}

// 計算四分位數 Q1 和 Q3
void computeQuartiles(int *data, int size, int *q1, int *q3) {
    if (size <= 0) {
        *q1 = *q3 = 0;
        return;
    }
    *q1 = data[size / 4];
    *q3 = data[3 * size / 4];
}

// 計算四分位距（IQR）及下邊界（LB）、上邊界（UB）
void computeIQRBounds(int q1, int q3, int min, int max, int *iqr, int *lb, int *ub) {
    *iqr = q3 - q1;
    *lb = (min > (q1 - 1.5 * (*iqr))) ? min : (q1 - 1.5 * (*iqr));
    *ub = (max < (q3 + 1.5 * (*iqr))) ? max : (q3 + 1.5 * (*iqr));
}

// 計算標準差
double computeStandardDeviation(int *data, int size, double avg) {
    if (size <= 0) return 0.0;

    double variance = 0;
    for (int i = 0; i < size; i++) {
        variance += pow(data[i] - avg, 2);
    }
    return sqrt(variance / size);
}

// 主函式：計算所有統計數據
void calculateStatistics(int *data, int size, int *min, int *max, double *avg, 
                         int *q1, int *q3, int *iqr, int *lb, int *ub, 
                         double *stddev, double *median) {
    if (size <= 0) {
        *min = *max = *q1 = *q3 = *iqr = *lb = *ub = 0;
        *avg = *stddev = *median = 0.0;
        return;
    }

    if (size == 1) {
        *min = *max = *q1 = *q3 = *iqr = *lb = *ub = data[0];
        *avg = *stddev = *median = data[0];
        return;
    }

    // **排序數據**
    qsort(data, size, sizeof(int), compareInt);

    // **計算最小值 & 最大值**
    computeMinMax(data, size, min, max);

    // **計算平均值**
    *avg = computeAverage(data, size);

    // **計算 Q1 & Q3**
    computeQuartiles(data, size, q1, q3);

    // **計算四分位距 IQR**
    computeIQRBounds(*q1, *q3, *min, *max, iqr, lb, ub);

    // **計算標準差**
    *stddev = computeStandardDeviation(data, size, *avg);

    // **計算中位數**
    if (size % 2 == 1) { 
        *median = data[size / 2];  // 若是奇數，取中間的值
    } else { 
        *median = (data[size / 2 - 1] + data[size / 2]) / 2.0; // 若是偶數，取中間兩個數的平均值
    }
}


void printEventStatsPage(LinkedList *list, int timeType, const char *title) {
    printf("\n%s\n\n", title);
    printf("Call Type                 |  count |  min  |  LB   |  Q1   |  med  |  mean   |  Q3   |  UB   |  max  | IQR | stddev\n");
    printf("------------------------------------------------------------------------------------------------------------\n");

    DoubleLinkedList *cur = list->headNode;
    while (cur) {
        int *arr = NULL;
        int size = 0;

        // **選擇不同的時間差數據**
        if (timeType == 1) {
            arr = cur->dispatchReceivedArr;
            size = cur->drSize;
        } else if (timeType == 2) {
            arr = cur->onsceneEnrouteArr;
            size = cur->oeSize;
        } else if (timeType == 3) {
            arr = cur->onsceneReceivedArr;
            size = cur->orSize;
        }

        if (size > 0) {
            // **計算統計數據**
            int minVal, maxVal, q1, q3, iqr, lb, ub;
            double avgVal, stddev, medianVal;

            calculateStatistics(arr, size, &minVal, &maxVal, &avgVal, 
                                &q1, &q3, &iqr, &lb, &ub, &stddev, &medianVal);

            // **輸出當前事件的統計**
            printf("%-25s | %6d | %5d | %5d | %5d | %5.0f | %8.2f | %5d | %5d | %5d | %3d | %7.2f\n",
                   cur->nameRecord,
                   cur->data,
                   minVal,
                   lb,
                   q1,
                   medianVal,  // **輸出中位數**
                   avgVal,
                   q3,
                   ub,
                   maxVal,
                   iqr,
                   stddev);
        }
        cur = cur->nextNode;
    }
    printf("\n");
}


