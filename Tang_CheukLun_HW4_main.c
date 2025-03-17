#define _XOPEN_SOURCE 700
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include "Tang_CheukLun_HW4_datastruct.h"
#include <strings.h>
#include <time.h>

#define BUFFER 1024
#define DATETIME_FORMAT "%m/%d/%Y %I:%M:%S %p"
#define BATCH_SIZE 10000

// 合併統計資料，依據課本“join on threads”概念，主執行緒在所有工作 thread 完成後再合併資料
void mergeEventStats(LinkedList *mainList, LinkedList *threadList)
{
    DoubleLinkedList *newList = threadList->headNode;
    while (newList)
    {
        // 遍歷 list1 (dispatchReceived)
        for (int i = 0; i < newList->size1; i++)
        {
            insertData(mainList, newList->nameRecord, newList->list1[i], 0, 0);
        }
        // 遍歷 list2 (onsceneEnroute)
        for (int i = 0; i < newList->size2; i++)
        {
            insertData(mainList, newList->nameRecord, 0, newList->list2[i], 0);
        }
        // 遍歷 list3 (onsceneReceived)
        for (int i = 0; i < newList->size3; i++)
        {
            insertData(mainList, newList->nameRecord, 0, 0, newList->list3[i]);
        }
        newList = newList->nextNode;
    }
}

int parseDatetime(const char *datetime_str, struct tm *out_tm)
{
    if (!datetime_str || strlen(datetime_str) < 19)
        return 0;
    memset(out_tm, 0, sizeof(struct tm));
    if (strptime(datetime_str, DATETIME_FORMAT, out_tm) == NULL)
        return 0;
    return 1;
}

time_t convertToEpoch(struct tm *time_struct)
{
    if (!time_struct)
        return -1;
    time_struct->tm_isdst = -1;
    return mktime(time_struct);
}

int calculateTimeDiff(const char *time1_str, const char *time2_str)
{
    struct tm tm1, tm2;
    time_t epoch1, epoch2;

    if (!time1_str || strlen(time1_str) == 0 || !time2_str || strlen(time2_str) == 0)
        return -1;
    if (!parseDatetime(time1_str, &tm1) || !parseDatetime(time2_str, &tm2))
        return -1;
    tm1.tm_isdst = -1;
    tm2.tm_isdst = -1;
    epoch1 = convertToEpoch(&tm1);
    epoch2 = convertToEpoch(&tm2);
    if (epoch1 == -1 || epoch2 == -1)
    {
        printf("Error: Failed to convert time to epoch.\n");
        return -1;
    }
    double diff = difftime(epoch2, epoch1);
    if (diff < 0)
        diff = -diff;
    return (int)diff;
}

// 每個 thread 根據分配的記錄區間獨立進行檔案解析與統計資料計算
void *parseLawThread(void *arg)
{
    ThreadArg *tArg = (ThreadArg *)arg;
    LinkedList *eventStatsTotal = createList();
    LinkedList *eventStatsSub1 = createList();
    LinkedList *eventStatsSub2 = createList();

    int file = open(tArg->filename, O_RDONLY);
    if (file == -1)
        return NULL;

    long current = tArg->startRecord;
    while (current < tArg->endRecord)
    {
        long batchCount = (tArg->endRecord - current < BATCH_SIZE) ? tArg->endRecord - current : BATCH_SIZE;
        size_t bytesToRead = batchCount * tArg->lengthPerRecord;
        char *buffer = (char *)malloc(bytesToRead + 1);
        if (!buffer)
        {
            printf("Thread [%ld-%ld]: malloc failed.\n", tArg->startRecord, tArg->endRecord);
            close(file);
            return NULL;
        }
        off_t offset = current * tArg->lengthPerRecord;
        ssize_t rd = pread(file, buffer, bytesToRead, offset);
        if (rd <= 0)
        {
            free(buffer);
            break;
        }
        buffer[rd] = '\0';

        for (int i = 0; i < batchCount; i++)
        {
            char *recordPtr = buffer + i * tArg->lengthPerRecord;
            char received_datetime[25] = "";
            char dispatch_datetime[25] = "";
            char enroute_datetime[25] = "";
            char onscene_datetime[25] = "";
            char call_type_final_desc[50] = "";
            char call_type_original_desc[50] = "";
            char subfieldValue[64] = "";

            int offsetCounter = 0;
            DoubleLinkedList *node = tArg->headerList->headNode;

            while (node)
            {
                char value[node->data + 1];
                size_t j = 0;

                while (j < node->data && recordPtr[offsetCounter + j] != '\0')
                {
                    value[j] = recordPtr[offsetCounter + j];
                    j++;
                }
                value[j] = '\0';

                char *start = value;
                while (*start == ' ')
                    start++;
                char *end = start + strlen(start) - 1;
                while (end > start && *end == ' ')
                    *end-- = '\0';

                // 透過 `if-else` 判斷該將 `value` 放入哪個變數
                char *destination = NULL;
                size_t maxSize = 0;

                if (checkEqual(node->nameRecord, "received_datetime"))
                {
                    destination = received_datetime;
                    maxSize = sizeof(received_datetime);
                }
                else if (checkEqual(node->nameRecord, "dispatch_datetime"))
                {
                    destination = dispatch_datetime;
                    maxSize = sizeof(dispatch_datetime);
                }
                else if (checkEqual(node->nameRecord, "enroute_datetime"))
                {
                    destination = enroute_datetime;
                    maxSize = sizeof(enroute_datetime);
                }
                else if (checkEqual(node->nameRecord, "onscene_datetime"))
                {
                    destination = onscene_datetime;
                    maxSize = sizeof(onscene_datetime);
                }
                else if (checkEqual(node->nameRecord, "call_type_final_desc"))
                {
                    destination = call_type_final_desc;
                    maxSize = sizeof(call_type_final_desc);
                }
                else if (checkEqual(node->nameRecord, "call_type_original_desc"))
                {
                    destination = call_type_original_desc;
                    maxSize = sizeof(call_type_original_desc);
                }
                else if (checkEqual(node->nameRecord, tArg->subfieldName))
                {
                    destination = subfieldValue;
                    maxSize = sizeof(subfieldValue);
                }

                // 若 `destination` 不為 `NULL`，則複製字串
                if (destination != NULL)
                {
                    size_t k = 0;
                    while (k < maxSize - 1 && start[k] != '\0')
                    {
                        destination[k] = start[k];
                        k++;
                    }
                    destination[k] = '\0'; // 確保字串結尾安全
                }

                offsetCounter += node->data;
                node = node->nextNode;
            }

            int diffDR = calculateTimeDiff(dispatch_datetime, received_datetime);
            int diffOE = calculateTimeDiff(onscene_datetime, enroute_datetime);
            int diffOR = calculateTimeDiff(onscene_datetime, received_datetime);

            if (diffDR != -1 || diffOE != -1 || diffOR != -1)
            {
                char eventType[64];
                size_t m = 0;
                const char *src = (strlen(call_type_final_desc) > 0) ? call_type_final_desc : call_type_original_desc;

                while (m < sizeof(eventType) - 1 && src[m] != '\0')
                {
                    eventType[m] = src[m];
                    m++;
                }
                eventType[m] = '\0';

                insertData(eventStatsTotal, eventType, diffDR, diffOE, diffOR);
                if (checkEqual(subfieldValue, tArg->subVal1))
                    insertData(eventStatsSub1, eventType, diffDR, diffOE, diffOR);
                if (checkEqual(subfieldValue, tArg->subVal2))
                    insertData(eventStatsSub2, eventType, diffDR, diffOE, diffOR);
            }
        }

        free(buffer);
        current += batchCount;
    }
    close(file);

    tArg->eventStatsTotal = eventStatsTotal;
    tArg->eventStatsSub1 = eventStatsSub1;
    tArg->eventStatsSub2 = eventStatsSub2;

    printf("Thread [%ld-%ld] finished processing %ld records.\n",
           tArg->startRecord, tArg->endRecord, tArg->endRecord - tArg->startRecord);
    pthread_exit(NULL);
}

void parseFile(const char *filename, LinkedList *list)
{
    int header = open(filename, O_RDONLY);
    if (header == -1)
    {
        printf("can not open the file");
        return;
    }
    struct stat headerStat;
    if (fstat(header, &headerStat) == -1)
    {
        printf("can not get header stat");
        close(header);
        return;
    }
    size_t statSize = headerStat.st_size;
    char *storage = malloc(statSize + 1);
    size_t readData = read(header, storage, statSize);
    if (readData == -1)
    {
        close(header);
        return;
    }
    storage[readData] = '\0';
    close(header);
    char *current = storage;
    while (*current != '\0')
    {
        char *colon = strchr(current, ':');
        if (!colon)
            break;
        *colon = '\0';
        int width = atoi(current);
        char *name = colon + 1;
        while (*name == ' ')
            name++;
        char *endOfLine = strchr(name, '\n');
        if (endOfLine)
        {
            *endOfLine = '\0';
            current = endOfLine + 1;
        }
        else
        {
            current = name + strlen(name);
        }
        DoubleLinkedList *checkNode = list->headNode;
        int exists = 0;
        while (checkNode)
        {
            if (strcmp(checkNode->nameRecord, name) == 0)
            {
                exists = 1;
                break;
            }
            checkNode = checkNode->nextNode;
        }
        if (!exists)
            insertAtTail(list, name, width);
        else
            printf("[INFO] Skipping duplicate field: %s\n", name);
    }
    free(storage);
}

void printAllPages(LinkedList *eventStatsTotal, LinkedList *eventStatsSub1, LinkedList *eventStatsSub2,
                   const char *subVal1, const char *subVal2, const char *subType)
{
    char totalTitle[128];
    snprintf(totalTitle, sizeof(totalTitle), "TOTAL - %s", subType);
    printPage(eventStatsTotal, totalTitle, subType);
    char title1[128], title2[128];
    snprintf(title1, sizeof(title1), "%s - %s", subVal1, subType);
    snprintf(title2, sizeof(title2), "%s - %s", subVal2, subType);
    printPage(eventStatsSub1, title1, subType);
    printPage(eventStatsSub2, title2, subType);
}

int main(int argc, char *argv[])
{

    char *dataName = strdup(argv[1]); //<program>

    char *headerName = strdup(argv[2]); // <data filename>

    int threads = atoi(argv[3]); //<header filename>

    char *subfieldName = strdup(argv[4]); //<threads>

    char *subFieldValue1 = strdup(argv[5]); //<subfield>

    char *subFieldValue2 = strdup(argv[6]); //<subfield value 1>

    LinkedList *newList = createList(); // create a new list to calulate the header.txt length

    if (newList == NULL)
    { // null check
        return 1;
    }

    parseFile(headerName, newList); // parse header.txt and then add back to new list

    int length = calulateLength(newList); // calculate the length of parsed data

    int law = open(dataName, O_RDONLY); // open 500k.dat

    struct stat status;

    fstat(law, &status);

    long totalRecords = status.st_size / length;

    close(law);

    //  Create three separate linkedlist to keep statistics
    LinkedList *totalEvent = createList();
    LinkedList *eventStatsSub1 = createList();
    LinkedList *eventStatsSub2 = createList();

    // 計時開始
    struct timespec startTime, endTime;
    clock_gettime(CLOCK_REALTIME, &startTime);

    // 依照課本方式使用 pthread_attr_t 設置預設屬性
    pthread_attr_t attr;
    pthread_attr_init(&attr); // 使用預設屬性

    // 建立 thread 並分配記錄區間
    if (threads < 1)
        threads = 1;
    pthread_t *tid = malloc(sizeof(pthread_t) * threads);
    ThreadArg *tArgs = malloc(sizeof(ThreadArg) * threads);
    long recordsPerThread = totalRecords / threads;
    long remainder = totalRecords % threads;
    long startRec = 0;
    for (int i = 0; i < threads; i++)
    {
        long endRec = startRec + recordsPerThread;
        if (i == threads - 1)
            endRec += remainder;
        tArgs[i].filename = dataName;
        tArgs[i].headerList = newList;
        tArgs[i].eventStatsTotal = totalEvent;
        tArgs[i].eventStatsSub1 = eventStatsSub1;
        tArgs[i].eventStatsSub2 = eventStatsSub2;
        tArgs[i].subfieldName = subfieldName;
        tArgs[i].subVal1 = subFieldValue1;
        tArgs[i].subVal2 = subFieldValue2;
        tArgs[i].lengthPerRecord = length;
        tArgs[i].startRecord = startRec;
        tArgs[i].endRecord = endRec;
        // 依照課本示範，傳入 attr 參數（此處使用預設屬性）
        pthread_create(&tid[i], &attr, parseLawThread, &tArgs[i]);
        startRec = endRec;
    }
    // 回收所有 thread（參照 Figure 4.12 範例）
    for (int i = 0; i < threads; i++)
        pthread_join(tid[i], NULL);

    // 合併各 thread 結果，並釋放各自使用的清單
    for (int i = 0; i < threads; i++)
    {
        mergeEventStats(totalEvent, tArgs[i].eventStatsTotal);
        mergeEventStats(eventStatsSub1, tArgs[i].eventStatsSub1);
        mergeEventStats(eventStatsSub2, tArgs[i].eventStatsSub2);
        releaseLinkedList(tArgs[i].eventStatsTotal);
        releaseLinkedList(tArgs[i].eventStatsSub1);
        releaseLinkedList(tArgs[i].eventStatsSub2);
    }

    // 顯示統計結果
    const char *subTypes[] = {"dispatchReceived", "onsceneEnroute", "onsceneReceived"};
    for (int i = 0; i < 3; i++)
    {
        printAllPages(totalEvent, eventStatsSub1, eventStatsSub2, subFieldValue1, subFieldValue2, subTypes[i]);
    }

    // 計時結束
    clock_gettime(CLOCK_REALTIME, &endTime);
    time_t sec = endTime.tv_sec - startTime.tv_sec;
    long n_sec = endTime.tv_nsec - startTime.tv_nsec;
    if (endTime.tv_nsec < startTime.tv_nsec)
    {
        sec--;
        n_sec += 1000000000L;
    }
    printf("Total Time was %ld.%09ld seconds\n", sec, n_sec);

    free(tid);
    free(tArgs);
    free(dataName);
    free(headerName);
    free(subfieldName);
    free(subFieldValue1);
    free(subFieldValue2);
    pthread_attr_destroy(&attr);
    // 根據需求，可釋放 newList 及其他全域清單
    pthread_exit(NULL);
    return 0;
}
