#include <stdio.h>
#include "Tang_CheukLun_HW4_datastruct.h"
#include <string.h>
#include <math.h>
#include <pthread.h>

LinkedList *createList()
{
    LinkedList *newNode = (LinkedList *)malloc(sizeof(LinkedList));
    if (!newNode)
    {
        printf("The Node is empty\n");
        return NULL;
    }
    newNode->headNode = NULL;
    newNode->tailNode = NULL;
    pthread_mutex_init(&newNode->lock, NULL);
    return newNode;
}

int calulateLength(LinkedList *list)
{
    int total = 0;
    DoubleLinkedList *curr = list->headNode;
    while (curr)
    {
        total += curr->data;
        curr = curr->nextNode;
    }
    return total;
}

void insertAtTail(LinkedList *list, const char *name, int width)
{

    DoubleLinkedList *newNode = (DoubleLinkedList *)malloc(sizeof(DoubleLinkedList));
    if (!newNode)
    {
        printf("The Node is empty");
        return;
    }

    newNode->nameRecord = malloc(strlen(name) + 1);
    if (!newNode->nameRecord)
    {
        printf("Memory allocation for nameRecord failed\n");
        free(newNode);

        return;
    }
    strcpy(newNode->nameRecord, name);
    newNode->data = width;
    newNode->nextNode = NULL;

    if (!list->headNode)
    {
        newNode->prevNode = NULL;
        list->headNode = newNode;
        list->tailNode = newNode;
    }
    else
    {
        newNode->prevNode = list->tailNode;
        list->tailNode->nextNode = newNode;
        list->tailNode = newNode;
    }
}

void releaseLinkedList(LinkedList *list)
{
    if (!list)
        return;
    // 遍歷釋放所有節點
    DoubleLinkedList *curr = list->headNode;
    while (curr)
    {
        DoubleLinkedList *nextNode = curr->nextNode;
        if (curr->nameRecord)
            free(curr->nameRecord);
        if (curr->list1)
            free(curr->list1);
        if (curr->list2)
            free(curr->list2);
        if (curr->list3)
            free(curr->list3);
        free(curr);
        curr = nextNode;
    }
    // 銷毀 mutex
    pthread_mutex_destroy(&list->lock);
    free(list);
}
// write a simple function to compare
int checkEqual(const char *firstString, const char *secondString)
{

    if (firstString == NULL || secondString == NULL)
    {
        return 0;
    }

    int index = 0;
    while (firstString[index] != '\0' && secondString[index] != '\0')
    {
        if (firstString[index] != secondString[index])
        {
            return 0;
        }
        index++;
    }

    if (firstString[index] == '\0' && secondString[index] == '\0')
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

void insertData(LinkedList *eventStatsList, const char *eventType, int dispatchToReceivedDiff,
                int enrouteToOnsceneDiff, int receivedToOnsceneDiff)
{

    if (eventStatsList == NULL || eventType == NULL)
    {
        // If the linked list or event type is invalid, exit early
        return;
    }

    // Lock the list for multiple threads
    pthread_mutex_lock(&eventStatsList->lock);

    // Search for an existing node matching the event type
    DoubleLinkedList *currentNode = eventStatsList->headNode;

    int isNodeFound = 0; // Flag to track

    while (currentNode != NULL)
    {
        if (checkEqual(currentNode->nameRecord, eventType))
        {
            // We found a node that matches the event type
            isNodeFound = 1;
            break;
        }
        currentNode = currentNode->nextNode;
    }

    // If no matching node was found
    if (isNodeFound == 0)
    {
        insertAtTail(eventStatsList, eventType, 0);

        currentNode = eventStatsList->tailNode;

        // If the new node is created successfully, initialize its arrays
        if (currentNode != NULL)
        {
            currentNode->list1 = NULL;
            currentNode->size1 = 0;
            currentNode->maxSize1 = 0;

            currentNode->list2 = NULL;
            currentNode->size2 = 0;
            currentNode->maxSize2 = 0;

            currentNode->list3 = NULL;
            currentNode->size3 = 0;
            currentNode->maxSize3 = 0;
        }
    }

    // Step 3: Update statistics if the node is valid
    if (currentNode != NULL)
    {
        // Increase the general counter for how many times this event is recorded
        currentNode->data++;

        // --- Expand and insert dispatchToReceivedDiff into list1 ---
        if (currentNode->maxSize1 == 0)
        {
            currentNode->maxSize1 = 10;
            currentNode->list1 = (int *)malloc(sizeof(int) * currentNode->maxSize1);
            currentNode->size1 = 0;
        }
        else if (currentNode->size1 >= currentNode->maxSize1)
        {
            currentNode->maxSize1 *= 3;
            currentNode->list1 = (int *)realloc(
                currentNode->list1,
                sizeof(int) * currentNode->maxSize1);
        }
        currentNode->list1[currentNode->size1] = dispatchToReceivedDiff;
        currentNode->size1++;

        // --- Expand and insert enrouteToOnsceneDiff into list2 ---
        if (currentNode->maxSize2 == 0)
        {
            currentNode->maxSize2 = 10;
            currentNode->list2 = (int *)malloc(sizeof(int) * currentNode->maxSize2);
            currentNode->size2 = 0;
        }
        else if (currentNode->size2 >= currentNode->maxSize2)
        {
            currentNode->maxSize2 *= 3;
            currentNode->list2 = (int *)realloc(
                currentNode->list2,
                sizeof(int) * currentNode->maxSize2);
        }
        currentNode->list2[currentNode->size2] = enrouteToOnsceneDiff;
        currentNode->size2++;

        // --- Expand and insert receivedToOnsceneDiff into list3 ---
        if (currentNode->maxSize3 == 0)
        {
            currentNode->maxSize3 = 10;
            currentNode->list3 = (int *)malloc(sizeof(int) * currentNode->maxSize3);
            currentNode->size3 = 0;
        }
        else if (currentNode->size3 >= currentNode->maxSize3)
        {
            currentNode->maxSize3 *= 3;
            currentNode->list3 = (int *)realloc(
                currentNode->list3,
                sizeof(int) * currentNode->maxSize3);
        }
        currentNode->list3[currentNode->size3] = receivedToOnsceneDiff;
        currentNode->size3++;
    }

    // Unlock the list
    pthread_mutex_unlock(&eventStatsList->lock);
}

int bubble_sort_inner(int A[], int N)
{
    int shift;
    int swapped = 0;

    int i = 0;
    while (i < N - 1)
    {
        if (A[i] > A[i + 1])
        {
            shift = A[i];
            A[i] = A[i + 1];
            A[i + 1] = shift;
            swapped = 1;
        }
        i++;
    }
    return swapped;
}

void bubble_sort(int A[], int N)
{
    int sorted = 1;
    while (sorted)
    {
        sorted = bubble_sort_inner(A, N);
        N--;
    }
}

void printPage(LinkedList *list, char *title, const char *sub)
{

    printf("\n%s\n\n", title);
    printf("       Call Type        | count  |  min   |   LB   |   Q1   |  med   |  mean    |  Q3   | UB    | max    |  IQR  |   stddev\n");

    DoubleLinkedList *currentNode = list->headNode;

    while (currentNode)
    {
        int *timeDifferences = NULL;
        int timeDataCount = 0;

        if (currentNode->list1 != NULL && currentNode->size1 > 0)
        {
            timeDifferences = currentNode->list1;
            timeDataCount = currentNode->size1;
        }
        else if (currentNode->list2 != NULL && currentNode->size2 > 0)
        {
            timeDifferences = currentNode->list2;
            timeDataCount = currentNode->size2;
        }
        else if (currentNode->list3 != NULL && currentNode->size3 > 0)
        {
            timeDifferences = currentNode->list3;
            timeDataCount = currentNode->size3;
        }

        if (timeDifferences != NULL && timeDataCount > 0)
        {
            bubble_sort(timeDifferences, timeDataCount);

            int minTime = timeDifferences[0];
            int maxTime = timeDifferences[timeDataCount - 1];

            double median;
            int index = (timeDataCount + 1) / 2 - 1;
            if (timeDataCount % 2 == 1)
            {
                median = timeDifferences[index];
            }
            else
            {
                median = (timeDifferences[index] + timeDifferences[index + 1]) / 2;
            }

            double avgTime = 0;

            for (int i = 0; i < timeDataCount; i++)
            {
                avgTime += timeDifferences[i];
            }

            avgTime = avgTime / timeDataCount;

            int q1Index = (timeDataCount + 1) * 0.25 - 1;
            int q3Index = (timeDataCount + 1) * 0.75 - 1;

            if (q1Index < 0)
            {
                q1Index = 0;
            }
            if (q3Index >= timeDataCount)
            {
                q3Index = timeDataCount - 1;
            }

            int q1 = timeDifferences[q1Index];
            int q3 = timeDifferences[q3Index];

            int IQR = q3 - q1;

            int lowerBound = q1 - (1.5 * IQR);
            int upperBound = q3 + (1.5 * IQR);

            if (lowerBound < minTime)
            {
                lowerBound = minTime;
            }

            if (upperBound > maxTime)
            {
                upperBound = maxTime;
            }

            double sum = 0;

            for (int i = 0; i < timeDataCount; i++)
            {
                double difference = timeDifferences[i] - avgTime;
                sum += difference * difference;
            }

            double stdDev = 0;
            if (timeDataCount > 0)
            {
                stdDev = sqrt(sum / timeDataCount);
            }

            printf("%23s | %6d | %6d | %6d | %6d | %6.0f | %8.2f | %5d | %5d | %6d | %5d | %7.2f\n",
                   currentNode->nameRecord, currentNode->data, minTime, lowerBound, q1,
                   median, avgTime, q3, upperBound, maxTime, IQR, stdDev);
        }

        currentNode = currentNode->nextNode;
    }
    printf("\n");
}