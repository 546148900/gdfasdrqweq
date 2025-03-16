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
#include <pthread.h> 




#define BUFFER 1024
#define DATETIME_FORMAT "%m/%d/%Y %I:%M:%S %p"
#define BATCH_SIZE 10000

extern pthread_mutex_t gListMutex; 

pthread_mutex_t gListMutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    const char* filename;             // 資料檔案
    LinkedList* headerList;           // Header List (欄位定義)
    LinkedList* eventStatsTotal;      // 全城市
    LinkedList* eventStatsSub1;       // 子欄位1
    LinkedList* eventStatsSub2;       // 子欄位2
    const char* subfieldName;         // 例如 "police_district"
    const char* subVal1;             // 例如 "BAYVIEW"
    const char* subVal2;             // 例如 "MISSION"
    int lengthPerRecord;              // 單筆紀錄長度
    long startRecord;                 // 要處理的起始筆 (含)
    long endRecord;                   // 要處理的結束筆 (不含)
} ThreadArg;


void mergeEventStats(LinkedList *mainList, LinkedList *threadList) {
    DoubleLinkedList *cur = threadList->headNode;
    while (cur) {
        insertOrUpdateEventStats(mainList, cur->nameRecord, 
                                 cur->dispatchReceivedArr[0], 
                                 cur->onsceneEnrouteArr[0], 
                                 cur->onsceneReceivedArr[0]);
        cur = cur->nextNode;
    }
}


int parseDatetime(const char *datetime_str, struct tm *out_tm) {
    if (!datetime_str || strlen(datetime_str) < 19) {
        
        return 0;
    }

    memset(out_tm, 0, sizeof(struct tm));  // 初始化 tm 結構
    if (strptime(datetime_str, DATETIME_FORMAT, out_tm) == NULL) {
      
        return 0;
    }

    return 1;
}



time_t convertToEpoch(struct tm *time_struct) {
    if (!time_struct) return -1;
     time_struct->tm_isdst = -1;
    return mktime(time_struct);
}


int calculateTimeDiff(const char *time1_str, const char *time2_str) {
    struct tm tm1, tm2;
    time_t epoch1, epoch2;

    // 檢查輸入是否為 NULL 或空字串
    if (!time1_str || strlen(time1_str) == 0 || !time2_str || strlen(time2_str) == 0) {
        return -1;
    }

    // 解析時間字符串
    if (!parseDatetime(time1_str, &tm1) || !parseDatetime(time2_str, &tm2)) {
        return -1;  // 若解析錯誤，返回 -1
    }

    // 設置 tm_isdst = -1 讓 mktime() 自動調整夏令時
    tm1.tm_isdst = -1;
    tm2.tm_isdst = -1;

    // 轉換成 epoch 時間戳
    epoch1 = convertToEpoch(&tm1);
    epoch2 = convertToEpoch(&tm2);

    if (epoch1 == -1 || epoch2 == -1) {
        printf("Error: Failed to convert time to epoch.\n");
        return -1;
    }


    // 計算時間差，確保結果為正
    double diff = difftime(epoch2, epoch1);
    if (diff < 0) {
        diff = -diff;
    }

    return (int)diff;
}

void* parseLawThread(void* arg) {
    ThreadArg* tArg = (ThreadArg*)arg;

    // 每個執行緒建立自己的 LinkedList
    LinkedList* eventStatsTotal = createList();
    LinkedList* eventStatsSub1 = createList();
    LinkedList* eventStatsSub2 = createList();

    int file = open(tArg->filename, O_RDONLY);
    if (file == -1) {
        printf("Thread[%ld-%ld]: Cannot open file %s\n", tArg->startRecord, tArg->endRecord, tArg->filename);
        return NULL;
    }

    long current = tArg->startRecord;
    while (current < tArg->endRecord) {
        long batchCount = (tArg->endRecord - current < BATCH_SIZE) ? tArg->endRecord - current : BATCH_SIZE;
        size_t bytesToRead = batchCount * tArg->lengthPerRecord;
        char* buffer = (char*)malloc(bytesToRead + 1);
        if (!buffer) {
            printf("Thread[%ld-%ld]: malloc failed.\n", tArg->startRecord, tArg->endRecord);
            close(file);
            return NULL;
        }

        off_t offset = current * tArg->lengthPerRecord;
        ssize_t rd = pread(file, buffer, bytesToRead, offset);
        if (rd <= 0) {
            free(buffer);
            break;
        }
        buffer[rd] = '\0';

        for (int i = 0; i < batchCount; i++) {
            char* recordPtr = buffer + i * tArg->lengthPerRecord;
            char received_datetime[25] = "";
            char dispatch_datetime[25] = "";
            char enroute_datetime[25] = "";
            char onscene_datetime[25] = "";
            char call_type_final_desc[50] = "";
            char call_type_original_desc[50] = "";
            char subfieldValue[64] = "";

            int offsetCounter = 0;
            DoubleLinkedList* node = tArg->headerList->headNode;
            while (node) {
                char value[node->data + 1];
                strncpy(value, recordPtr + offsetCounter, node->data);
                value[node->data] = '\0';

                char* start = value;
                while (*start == ' ') start++;
                char* end = start + strlen(start) - 1;
                while (end > start && *end == ' ') {
                    *end = '\0';
                    end--;
                }

                if (strcmp(node->nameRecord, "received_datetime") == 0)
                    strncpy(received_datetime, start, sizeof(received_datetime) - 1);
                else if (strcmp(node->nameRecord, "dispatch_datetime") == 0)
                    strncpy(dispatch_datetime, start, sizeof(dispatch_datetime) - 1);
                else if (strcmp(node->nameRecord, "enroute_datetime") == 0)
                    strncpy(enroute_datetime, start, sizeof(enroute_datetime) - 1);
                else if (strcmp(node->nameRecord, "onscene_datetime") == 0)
                    strncpy(onscene_datetime, start, sizeof(onscene_datetime) - 1);
                else if (strcmp(node->nameRecord, "call_type_final_desc") == 0)
                    strncpy(call_type_final_desc, start, sizeof(call_type_final_desc) - 1);
                else if (strcmp(node->nameRecord, "call_type_original_desc") == 0)
                    strncpy(call_type_original_desc, start, sizeof(call_type_original_desc) - 1);
                else if (strcmp(node->nameRecord, tArg->subfieldName) == 0)
                    strncpy(subfieldValue, start, sizeof(subfieldValue) - 1);

                offsetCounter += node->data;
                node = node->nextNode;
            }

            int diffDR = calculateTimeDiff(dispatch_datetime, received_datetime);
            int diffOE = calculateTimeDiff(onscene_datetime, enroute_datetime);
            int diffOR = calculateTimeDiff(onscene_datetime, received_datetime);

            if (diffDR != -1 || diffOE != -1 || diffOR != -1) {
                char eventType[64];
                if (strlen(call_type_final_desc) > 0)
                    strcpy(eventType, call_type_final_desc);
                else
                    strcpy(eventType, call_type_original_desc);

                insertOrUpdateEventStats(eventStatsTotal, eventType, diffDR, diffOE, diffOR);
                if (strcmp(subfieldValue, tArg->subVal1) == 0)
                    insertOrUpdateEventStats(eventStatsSub1, eventType, diffDR, diffOE, diffOR);
                if (strcmp(subfieldValue, tArg->subVal2) == 0)
                    insertOrUpdateEventStats(eventStatsSub2, eventType, diffDR, diffOE, diffOR);
            }
        }

        free(buffer);
        current += batchCount;
    }

    close(file);

    // 將結果存到 ThreadArg，讓主線程合併
    tArg->eventStatsTotal = eventStatsTotal;
    tArg->eventStatsSub1 = eventStatsSub1;
    tArg->eventStatsSub2 = eventStatsSub2;

    return NULL;
}





void parseFile(const char* filename, LinkedList *list){
    
    int header = open(filename,O_RDONLY);

    if(header == -1){
        
        printf("can not open the file");
        
        return;
    
    }

    struct stat headerStat;

 
    
    if(fstat(header,&headerStat)==-1){

        printf("can not get header stat");

        close(header);

        return;
    
    
    };
    size_t statSize = headerStat.st_size;
    
    char* storage = malloc(statSize + 1);

    size_t readData = read(header,storage,statSize); 


    if(readData == -1){
        
        close(header);
        
        return;
    
    }

    storage[readData] = '\0';
    
    close(header);


    char* current = storage;


    while (*current != '\0') {


    char* colon = strchr(current, ':');


    if(!colon){
        break;
    }

    *colon = '\0';


    int width = atoi(current);


     if (width <= 0) {
            printf("%s",current);
        }

     char* name = colon + 1;

      while (*name == ' '){
        name++; 
      } 

    char* endOfLine = strchr(name, '\n');


     if (endOfLine) {
        *endOfLine = '\0';
        current = endOfLine + 1;
    }
    else {
        current = name + strlen(name);  
    }

     DoubleLinkedList *checkNode = list->headNode;
        int exists = 0;
        while (checkNode) {
            if (strcmp(checkNode->nameRecord, name) == 0) {
                exists = 1;
                break;
            }
            checkNode = checkNode->nextNode;
        }


   if (!exists) {
            insertAtTail(list, name, width);
        } else {
            printf("[INFO] Skipping duplicate field: %s\n", name);
        }


    }

    

    free(storage);



}




   



int main (int argc, char *argv[]){
    //***TO DO***  Look at arguments, initialize application

    const char *dataFilename   = argv[1];
    const char *headerFilename = argv[2];
    int threads                = atoi(argv[3]);  // 之後Part3可能用
    const char *subfieldName   = argv[4];
    const char *subVal1        = argv[5];
    const char *subVal2        = argv[6];

    LinkedList* newList = createList();


    if(newList == NULL){

        return 1;
    
    }

    parseFile(headerFilename,newList);
    



    int length = calulateLength(newList);


    

    int law = open(dataFilename,O_RDONLY);
    
    struct stat status;

    int information = fstat(law,&status);

    long totalRecords = status.st_size / length; 

    printf("Total records in file: %ld\n", totalRecords);

   close(law);

LinkedList *eventStatsTotal = createList();
LinkedList *eventStatsSub1  = createList();
LinkedList *eventStatsSub2  = createList();




    

    //**************************************************************
    // DO NOT CHANGE THIS BLOCK
    //Time stamp start
    struct timespec startTime;
    struct timespec endTime;

    clock_gettime(CLOCK_REALTIME, &startTime);
    //**************************************************************
    
    // *** TO DO ***  start your thread processing
    //                wait for the threads to finish
 if(threads < 1) threads = 1;
    pthread_t *tid = malloc(sizeof(pthread_t)*threads);
    ThreadArg *tArgs = malloc(sizeof(ThreadArg)*threads);

    long recordsPerThread = totalRecords / threads;
    long remainder = totalRecords % threads;

    long startRec = 0;
    for(int i=0; i<threads; i++){
        long endRec = startRec + recordsPerThread;
        if(i == threads-1){
            endRec += remainder;  // 把餘數丟最後一個
        }
        // 填寫tArgs[i]
        tArgs[i].filename = dataFilename;
        tArgs[i].headerList = newList;
        tArgs[i].eventStatsTotal = eventStatsTotal;
        tArgs[i].eventStatsSub1  = eventStatsSub1;
        tArgs[i].eventStatsSub2  = eventStatsSub2;
        tArgs[i].subfieldName    = subfieldName;
        tArgs[i].subVal1         = subVal1;
        tArgs[i].subVal2         = subVal2;
        tArgs[i].lengthPerRecord = length;
        tArgs[i].startRecord     = startRec;
        tArgs[i].endRecord       = endRec;

        pthread_create(&tid[i], NULL, parseLawThread, &tArgs[i]);

        startRec = endRec;
    }

    // 等待所有 thread
    for(int i=0; i<threads; i++){
        pthread_join(tid[i], NULL);
    }


for(int i = 0; i < threads; i++) {
    mergeEventStats(eventStatsTotal, tArgs[i].eventStatsTotal);
    mergeEventStats(eventStatsSub1, tArgs[i].eventStatsSub1);
    mergeEventStats(eventStatsSub2, tArgs[i].eventStatsSub2);

    releaseLinkedList(tArgs[i].eventStatsTotal);
    releaseLinkedList(tArgs[i].eventStatsSub1);
    releaseLinkedList(tArgs[i].eventStatsSub2);
}



    // ***TO DO *** Display Data


printEventStatsPage(eventStatsTotal, 1, "TOTAL - Dispatch vs. Received");
printEventStatsPage(eventStatsTotal, 2, "TOTAL - OnScene vs. Enroute");
printEventStatsPage(eventStatsTotal, 3, "TOTAL - OnScene vs. Received");

// 2) 三頁 (eventStatsSub1) - 例如 "BAYVIEW"
printEventStatsPage(eventStatsSub1, 1, "BAYVIEW - Dispatch vs. Received");
printEventStatsPage(eventStatsSub1, 2, "BAYVIEW - OnScene vs. Enroute");
printEventStatsPage(eventStatsSub1, 3, "BAYVIEW - OnScene vs. Received");

// 3) 三頁 (eventStatsSub2) - 例如 "MISSION"
printEventStatsPage(eventStatsSub2, 1, "MISSION - Dispatch vs. Received");
printEventStatsPage(eventStatsSub2, 2, "MISSION - OnScene vs. Enroute");
printEventStatsPage(eventStatsSub2, 3, "MISSION - OnScene vs. Received");




    //**************************************************************
    // DO NOT CHANGE THIS BLOCK
    //Clock output
    clock_gettime(CLOCK_REALTIME, &endTime);
    time_t sec = endTime.tv_sec - startTime.tv_sec;
    long n_sec = endTime.tv_nsec - startTime.tv_nsec;
    if (endTime.tv_nsec < startTime.tv_nsec)
        {
        --sec;
        n_sec = n_sec + 1000000000L;
        }

    printf("Total Time was %ld.%09ld seconds\n", sec, n_sec);
    //**************************************************************


    // ***TO DO *** cleanup
    free(tid);
    free(tArgs);
    pthread_mutex_destroy(&gListMutex);

    return 0;
    }

