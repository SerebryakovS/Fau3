
#include "fau3vomlib.h"

redisContext *RStorage;

bool OpenObjectsStorage(){
    RStorage = redisConnect("127.0.0.1", 6379);
    if (RStorage != NULL && RStorage->err) {
        printf("[ERR][Redis]: Redis connection error: %s\n", RStorage->errstr);
        return false;
    } else {
        printf("[OK][Redis]: Connected to Redis storage\n");
        return true;
    };
};

void CloseObjectsStorage(){
    redisFree(RStorage);
};

bool AddStorageRecord(const char *ObjectUuid, unsigned char *SpeexPayload){
    redisReply *Reply;
    if ((Reply = redisCommand(RStorage, "SET %s %s", ObjectUuid, SpeexPayload)) == NULL){
        printf("[ERR][Redis]: Could not set new record: %s\n", RStorage->errstr);
        return false;
    } else {
        printf("[OK][Redis]: Stored new record with uuid=%s, Status:%s\n", ObjectUuid, Reply->str);
        freeReplyObject(Reply);
        return true;
    };
};

bool GetStorageRecord(const char *ObjectUuid){
    redisReply *Reply;
    if ((Reply = redisCommand(RStorage,"GET %s",ObjectUuid)) == NULL){
        printf("[OK][Redis]: Could not extract record with uuid=%s\n", ObjectUuid);
        return false;
    } else {
        printf("[OK][Redis]: Extracted record with uuid=%s, body=%s\n", ObjectUuid, Reply->str);
        freeReplyObject(Reply);
        return true;
    };
};

bool SetStorageObjectTimeout(const char *ObjectUuid, int Timeout){
    redisReply *Reply = redisCommand(RStorage, "EXPIRE %s %d", ObjectUuid, Timeout);
    if (Reply == NULL || Reply->type != REDIS_REPLY_INTEGER || Reply->integer == 0) {
        printf("[ERR][Redis]: Error setting key timeout: %s\n", RStorage->errstr);
        return false;
    };
    printf("[OK][Redis]: ObjectUuid %s set to expire in %d seconds\n", ObjectUuid, Timeout);
    freeReplyObject(Reply);
    return true;
};
