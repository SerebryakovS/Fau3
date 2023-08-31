
#include    "fau3vomlib.h"

bool OpenObjectsStorage(Fau3ConnectedClient *Client){
    Client->RStorageIO = redisConnect(REDIS_HOSTNAME, REDIS_PORT);
    if (Client->RStorageIO != NULL && Client->RStorageIO->err) {
        printf("[ERR][Redis]: Redis connection error: %s\n", Client->RStorageIO->errstr);
        return false;
    } else {
        printf("[OK][Redis]: Connected to Redis storage: %p\n",Client->RStorageIO);
        return true;
    };
};

void CloseObjectsStorage(Fau3ConnectedClient *Client){
    if (Client->RStorageIO != NULL){
        redisFree(Client->RStorageIO);
    };
    printf("[OK][Redis]: Disconnected from Redis storage: %p\n",Client->RStorageIO);
};

void AddStorageRecord(Fau3ConnectedClient *Client, char *ObjectUuid, char *Object, int Timeout){
    pthread_mutex_lock(&Client->StorageLock);
    OpenObjectsStorage(Client);
    redisReply *Reply;
    if ((Reply = redisCommand(Client->RStorageIO, "SET %s %s", ObjectUuid, Object)) == NULL){
        printf("[ERR][Redis]: Could not set new record: %s\n", Client->RStorageIO->errstr);
    } else {
        printf("[OK][Redis]: Stored new record with uuid=%s, Status:%s\n", ObjectUuid, Reply->str);
        if (Timeout > 0){
            freeReplyObject(Reply);
            Reply = redisCommand(Client->RStorageIO, "EXPIRE %s %d", ObjectUuid, Timeout);
            if (Reply == NULL || Reply->type != REDIS_REPLY_INTEGER || Reply->integer == 0) {
                printf("[ERR][Redis]: Error setting key timeout: %s\n", Client->RStorageIO->errstr);
            };
            printf("[OK][Redis]: ObjectUuid %s set to expire in %d seconds\n", ObjectUuid, Timeout);
        };
    };
    if (Reply){
        freeReplyObject(Reply);
    };
    CloseObjectsStorage(Client);
    pthread_mutex_unlock(&Client->StorageLock);
};

char *GetStorageRecord(Fau3ConnectedClient *Client, char *ObjectUuid, bool DeleteAfter){
    pthread_mutex_lock(&Client->StorageLock);
    OpenObjectsStorage(Client);
    redisReply *Reply;
    char *Object = NULL;
    if ((Reply = redisCommand(Client->RStorageIO,"GET %s",ObjectUuid)) == NULL){
        printf("[OK][Redis]: Could not extract record with uuid=%s\n", ObjectUuid);
    } else if (Reply->type == REDIS_REPLY_STRING){
        printf("[OK][Redis]: Extracted record with uuid=%s\n", ObjectUuid);
        if (DeleteAfter == true){
            redisCommand(Client->RStorageIO,"DEL %s",ObjectUuid);
        };
        Object = malloc(sizeof(char) * Reply->len);
        memcpy(Object, Reply->str, Reply->len);
    };
    if (Reply){
        freeReplyObject(Reply);
    };
    CloseObjectsStorage(Client);
    pthread_mutex_unlock(&Client->StorageLock);
    return Object;
};

char **FindObjectsKeysByClientId(Fau3ConnectedClient *Client, const char *ClientId, long *ObjectsCount) {
    pthread_mutex_lock(&Client->StorageLock);
    int CursorInt = 0; char **ObjectUuids; *ObjectsCount = 0;
    do {
        redisReply *Reply = redisCommand(Client->RStorageIO, "SCAN %d MATCH %s_*", CursorInt, ClientId);
        if (Reply == NULL || Reply->type != REDIS_REPLY_ARRAY) {
            printf("[ERR][Redis]: Error retrieving keys by ClientId: %s\n", Client->RStorageIO->errstr);
            ObjectUuids = NULL;
            break;
        };
        *ObjectsCount = Reply->element[1]->elements;
        if (*ObjectsCount > 0){
            char **ObjectUuids = (char **)calloc(*ObjectsCount, sizeof(char *));
            if (ObjectUuids == NULL){
                pthread_mutex_unlock(&Client->StorageLock);
                return NULL;
            };
            CursorInt = Reply->element[0]->integer;
            for (size_t Idx = 0; Idx < *ObjectsCount; Idx++) {
                const char *ObjectUuid = Reply->element[1]->element[Idx]->str;
                if (strstr(ObjectUuid, ClientId) != NULL) {
                    size_t StringLength = Reply->element[1]->element[Idx]->len;
                    ObjectUuids[Idx] = (char *)calloc(StringLength, sizeof(char));
                    strcpy(ObjectUuids[Idx],ObjectUuid);
                };
            };
            if (Reply){
                freeReplyObject(Reply);
            };
            pthread_mutex_unlock(&Client->StorageLock);
            return ObjectUuids;
        };
    } while (CursorInt != 0);
    pthread_mutex_unlock(&Client->StorageLock);
    return ObjectUuids;
};

void PublishMessageToStorage(Fau3ConnectedClient *Client, char *Object){
    pthread_mutex_lock(&Client->StorageLock);
    OpenObjectsStorage(Client);
    redisReply *Reply = redisCommand(Client->RStorageIO, "PUBLISH %s %s", Client->CurrentRoom, Object);
    if (Reply == NULL) {
        printf("[ERR]: Could not publish message: \n");
    }else{
        printf("[OK]: Message published to channel %s\n",  Client->CurrentRoom);
        freeReplyObject(Reply);
    };
    CloseObjectsStorage(Client);
    pthread_mutex_unlock(&Client->StorageLock);
};

void *SubscribeStorageMessages(void *_Client) {
    Fau3ConnectedClient *Client = (Fau3ConnectedClient *)_Client;
    // pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    Client->RStorageSubSignal = false;
    Client->RStorageSub = redisConnect(REDIS_HOSTNAME, REDIS_PORT);
    redisReply *Reply = redisCommand(Client->RStorageSub, "SUBSCRIBE %s", Client->CurrentRoom);
    if (Reply){
        freeReplyObject(Reply);
    };
    struct json_tokener *JsonTokener = json_tokener_new();
    while (1) {
        if (redisGetReply(Client->RStorageSub, (void **)&Reply) != REDIS_OK) {
            printf("[ERR]: Subscribe reading failed..\n");
        } else if (Reply->type == REDIS_REPLY_ARRAY && Reply->elements == 3) {
            if (strcmp(Reply->element[0]->str, "message") == 0) {
                const char *IncomingPacket = (const char *)Reply->element[2]->str;
                if (strcmp(IncomingPacket,"__stop__") == 0){
                    if (Reply){
                        freeReplyObject(Reply);
                    };
                    if (Client->RStorageSubSignal == true){
                        break;
                    };
                    continue;
                };
                json_object *IncomingJsonObject = json_tokener_parse_ex(JsonTokener,IncomingPacket,strlen(IncomingPacket));
                if (json_tokener_get_error(JsonTokener) == json_tokener_success) {
                    json_object *FieldObject = NULL;
                    json_object_object_get_ex(IncomingJsonObject, "ClientId", &FieldObject);
                    const char* ClientId = json_object_get_string(FieldObject);
                    if (strcmp(ClientId, Client->ClientId) != 0 || FAU3_ECHO_ENABLED > 0){
                        Fau3MessagesBufferPush(Client, Reply->element[2]->str);
                        lws_callback_on_writable_all_protocol(
                            lws_get_context ( Client->Websocket ),
                            lws_get_protocol( Client->Websocket ));
                    };
                };
                json_object_put(IncomingJsonObject);
                json_tokener_reset(JsonTokener);
                if (Reply){
                    freeReplyObject(Reply);
                };
            };
        };
    };
    json_tokener_free(JsonTokener);
    if (Client->RStorageSub != NULL){
        redisFree(Client->RStorageSub);
    };
    // pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    printf("[OK]: Subscription thread canceled: %s\n",Client->ClientId);
    return NULL;
};

void StopStorageSubscription(Fau3ConnectedClient *Client){
    pthread_t ExistingSubscription = Client->RStorageSubscriptionThread;
    if (ExistingSubscription != 0){
        if (pthread_kill(ExistingSubscription, 0) == 0){
            printf("[WARN]: Client is already subscribed. Stop running process..\n");
            Client->RStorageSubSignal = true;
            PublishMessageToStorage(Client,"__stop__");
            sleep(1);
            pthread_cancel(ExistingSubscription);
            pthread_join(ExistingSubscription, NULL);
        };
    };
};

