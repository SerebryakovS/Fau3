
#include    "fau3vomlib.h"


bool Fau3AppendClient(Fau3ActiveClients *ActiveClients, struct lws *WebsocketInstance, char *ClientId){
    Fau3ConnectedClient *Client = Fau3GetClientPtrById(ActiveClients, ClientId, NULL);
    if (Client){
        Client->Reconnected = true;
        lws_set_timeout(Client->Websocket, PENDING_TIMEOUT_AWAITING_PROXY_RESPONSE, LWS_TO_KILL_SYNC);
        Client->Websocket = WebsocketInstance;
        printf("[OK]: Connection recovered for Client:%s\n",ClientId);
        return true;
    };
    Fau3ConnectedClient NewClient;
    memcpy(NewClient.ClientId, ClientId, 18);
    NewClient.RStorageSubscriptionThread = 0;
    NewClient.Websocket = WebsocketInstance;
    NewClient.Fau3ClientMessages.Head = 0;
    NewClient.Fau3ClientMessages.Tail = 0;
    for (short Idx=0; Idx < FAU3_CLIENT_WAIT_MESSAGES_COUNT; Idx++){
        NewClient.Fau3ClientMessages.Buffer[Idx] = NULL;
    };
    NewClient.Fau3ClientMessages.TotalCount = 0;
    NewClient.IncomingBufferBytesCount = 0;
    NewClient.OutgoingBufferBytesCount = 0;
    NewClient.IncomingBuffer = NULL;
    NewClient.RStorageSub = NULL;
    NewClient.RStorageSubSignal = false;
    NewClient.RStorageIO = NULL;
    NewClient.OutgoingBuffer = NULL;
    NewClient.IncomingBuffer = NULL;
    NewClient.JsonTokener = json_tokener_new();
    NewClient.OutgoingJsonObject = NULL;
    NewClient.PeriodicPing = (PeriodicMessage){
        .Timeout  = 0,
        .SendFlag = false,
        .Counter  = 0
    };
    NewClient.PeriodicMonitor = (PeriodicMessage){
        .Timeout  = 0,
        .SendFlag = false,
        .Counter  = 0
    };
    time(&NewClient.ConnectedTime);
    NewClient.CachedMessages.Count = 0;
    pthread_mutex_init(&NewClient.StorageLock, NULL);
    pthread_mutex_init(&NewClient.InternalLock, NULL);
    pthread_mutex_lock(&ActiveClients->ClientsLock);
    long CurrentIndex = ActiveClients->TotalCount;
    if (CurrentIndex >= FAU3_CLIENTS_COUNT-1){
        printf("[ERR]: Maximum clients count reached..\n");
        pthread_mutex_unlock(&ActiveClients->ClientsLock);
        return false;
    };
    ActiveClients->Clients[CurrentIndex] = malloc(sizeof(Fau3ConnectedClient));
    memcpy(ActiveClients->Clients[CurrentIndex], &NewClient, sizeof(Fau3ConnectedClient));
    ActiveClients->TotalCount++;
    pthread_create(&ActiveClients->Clients[CurrentIndex]->CacheCheckerThread,
                   NULL, Fau3CheckOutgoingMessageCache, ActiveClients->Clients[CurrentIndex]);
    pthread_mutex_unlock(&ActiveClients->ClientsLock);
    printf("[OK]: Client object APPENDED. Current active clients count:%lu\n", ActiveClients->TotalCount);
    return true;
};

Fau3ConnectedClient *Fau3GetClientPtr(Fau3ActiveClients *ActiveClients, struct lws *WebsocketInstance, long *SearchingIndex){
    long _SearchingIndex = -1;
    Fau3ConnectedClient *ClientToReturn = NULL;
    pthread_mutex_lock(&ActiveClients->ClientsLock);
    for (long Idx=0; Idx < ActiveClients->TotalCount; Idx++){
        if (ActiveClients->Clients[Idx]->Websocket == WebsocketInstance){
            _SearchingIndex = Idx;
            break;
        };
    };
    if (_SearchingIndex >= 0){
        if (SearchingIndex != NULL){
            *SearchingIndex = _SearchingIndex;
        };
        ClientToReturn = ActiveClients->Clients[_SearchingIndex];
    };
    pthread_mutex_unlock(&ActiveClients->ClientsLock);
    return ClientToReturn;
};

Fau3ConnectedClient *Fau3GetClientPtrById(Fau3ActiveClients *ActiveClients, char* ClientId, long *SearchingIndex){
    long _SearchingIndex = -1;
    Fau3ConnectedClient *ClientToReturn = NULL;
    pthread_mutex_lock(&ActiveClients->ClientsLock);
    for (long Idx=0; Idx < ActiveClients->TotalCount; Idx++){
        if (strcmp(ActiveClients->Clients[Idx]->ClientId, ClientId) == 0) {
            _SearchingIndex = Idx;
            break;
        };
    };
    if (_SearchingIndex >= 0){
        if (SearchingIndex != NULL){
            *SearchingIndex = _SearchingIndex;
        };
        ClientToReturn = ActiveClients->Clients[_SearchingIndex];
    };
    pthread_mutex_unlock(&ActiveClients->ClientsLock);
    return ClientToReturn;
};

bool Fau3ClientIncomingBufferAppend(Fau3ActiveClients *ActiveClients, struct lws *WebsocketInstance, unsigned char *IncomingBuffer, long Length){
    bool IsPacketReceiveCompleted = false;
    long RemainingBytesCount = lws_remaining_packet_payload(WebsocketInstance);
    Fau3ConnectedClient *Client = Fau3GetClientPtr(ActiveClients,WebsocketInstance,NULL);
    if (Client){
        pthread_mutex_lock(&Client->InternalLock);
        if (lws_is_first_fragment(WebsocketInstance)) {
            if (Client->IncomingBuffer != NULL){
                free(Client->IncomingBuffer);
                Client->IncomingBuffer = NULL;
            };
            Client->IncomingBuffer = malloc(Length + RemainingBytesCount);
        };
        memcpy(&Client->IncomingBuffer[Client->IncomingBufferBytesCount],IncomingBuffer,Length);
        Client->IncomingBufferBytesCount += Length;
        if (RemainingBytesCount == 0){
            IsPacketReceiveCompleted = true;
            if (Fau3HandleStream(ActiveClients, Client) == false){
                Client->IncomingJsonObject = json_tokener_parse_ex(Client->JsonTokener,
                                                                   Client->IncomingBuffer,
                                                                   Client->IncomingBufferBytesCount);
                if (json_tokener_get_error(Client->JsonTokener) == json_tokener_success) {
                    Fau3HandleRequest(Client);
                    json_object_put(Client->IncomingJsonObject);
                };
                json_tokener_reset(Client->JsonTokener);
            };
            free(Client->IncomingBuffer);
            Client->IncomingBufferBytesCount = 0;
            Client->IncomingBuffer = NULL;
        };
        pthread_mutex_unlock(&Client->InternalLock);
    };
    return IsPacketReceiveCompleted;
};

void Fau3SetSubscriptionRequestsFlags(Fau3ActiveClients *ActiveClients, struct lws *WebsocketInstance){
    Fau3ConnectedClient *Client = Fau3GetClientPtr(ActiveClients,WebsocketInstance,NULL);
    if (Client){
        pthread_mutex_lock(&Client->InternalLock);
        if (Client->PeriodicPing.Timeout > 0){
            if (--Client->PeriodicPing.Counter == 0){
                if ( Client->PeriodicPing.SendFlag == true ){
                    printf("[OK]: PONG not received for client: %s\n",Client->ClientId);
                    if (FAU3_EXIT_ON_EMPTY_PONG > 0){
                        lws_set_timeout(WebsocketInstance,PENDING_TIMEOUT_AWAITING_PROXY_RESPONSE, LWS_TO_KILL_ASYNC);
                        printf("[OK]: FAU3_EXIT_ON_EMPTY_PONG set, so closing client:%s\n",Client->ClientId);
                    };
                    Client->PeriodicPing.SendFlag == false;
                } else {
                    lws_callback_on_writable(WebsocketInstance);
                    Client->PeriodicPing.SendFlag = true;
                    Client->PeriodicPing.Counter = Client->PeriodicPing.Timeout;
                };
            };
        };
        if (Client->PeriodicMonitor.Timeout > 0){
            if(--Client->PeriodicMonitor.Counter == 0){
                lws_callback_on_writable(WebsocketInstance);
                Client->PeriodicMonitor.SendFlag = true;
                Client->PeriodicMonitor.Counter = Client->PeriodicMonitor.Timeout;
            };
        };
        pthread_mutex_unlock(&Client->InternalLock);
    };
};

void Fau3GetSubscriptionRequestsFlags(Fau3ActiveClients *ActiveClients, struct lws *WebsocketInstance){
    Fau3ConnectedClient *Client = Fau3GetClientPtr(ActiveClients,WebsocketInstance,NULL);
    if (Client){
        pthread_mutex_lock(&Client->InternalLock);
        if (Client->PeriodicPing.SendFlag == true){
            Fau3Ping(Client);
        };
        if (Client->PeriodicMonitor.SendFlag == true){
            pthread_mutex_lock(&ActiveClients->ClientsLock);
            Fau3Monitor(ActiveClients, Client);
            pthread_mutex_unlock(&ActiveClients->ClientsLock);
            Client->PeriodicMonitor.SendFlag = false;
        };
        pthread_mutex_unlock(&Client->InternalLock);
    };
};

bool Fau3DeleteClient(Fau3ActiveClients *ActiveClients, struct lws *WebsocketInstance){
    long SearchingIndex = 0;
    long Idx = 0;
    Fau3ConnectedClient *Client = Fau3GetClientPtr(ActiveClients,WebsocketInstance,&SearchingIndex);
    if (Client){
        pthread_mutex_lock(&ActiveClients->ClientsLock);
        pthread_mutex_lock(&Client->InternalLock);
        if (Client->Reconnected == false){
            StopStorageSubscription(Client);
            for (Idx=0; Idx < Client->Fau3ClientMessages.TotalCount; Idx++){
                char *MessagesPtr = Client->Fau3ClientMessages.Buffer[Idx];
                if (MessagesPtr){
                    free(MessagesPtr);
                };
            };
            if (Client){
                free(Client);
            };
            for (Idx=SearchingIndex; Idx < ActiveClients->TotalCount-1; Idx++){
                ActiveClients->Clients[Idx] = ActiveClients->Clients[Idx+1];
            };
            json_tokener_free(Client->JsonTokener);
            ActiveClients->TotalCount--;
            pthread_mutex_destroy(&Client->StorageLock);
            pthread_mutex_unlock(&Client->InternalLock);
            pthread_mutex_destroy(&Client->InternalLock);
            printf("[DEL]: Client object DELETED. Current active clients count:%lu\n", ActiveClients->TotalCount);
        } else {
            Client->Reconnected = false;
            printf("[DEL]: Client object has been RECONNECTED, so nothing to do...\n");
            pthread_mutex_unlock(&Client->InternalLock);
        };
        pthread_mutex_unlock(&ActiveClients->ClientsLock);
    };
    return true;
};

void Fau3MessagesBufferPush(Fau3ConnectedClient *Client, char *IncomingSpeexMessage) {
    pthread_mutex_lock(&Client->StorageLock);
    if (Client->Fau3ClientMessages.TotalCount == FAU3_CLIENT_WAIT_MESSAGES_COUNT) {
        Client->Fau3ClientMessages.Tail = (Client->Fau3ClientMessages.Tail + 1) % FAU3_CLIENT_WAIT_MESSAGES_COUNT;
    } else {
        Client->Fau3ClientMessages.TotalCount++;
    };
    Client->Fau3ClientMessages.Buffer[Client->Fau3ClientMessages.Head] = (char *)calloc(strlen(IncomingSpeexMessage)+1, sizeof(char));
    strcpy(Client->Fau3ClientMessages.Buffer[Client->Fau3ClientMessages.Head],IncomingSpeexMessage);
    Client->Fau3ClientMessages.Head = (Client->Fau3ClientMessages.Head + 1) % FAU3_CLIENT_WAIT_MESSAGES_COUNT;
    pthread_mutex_unlock(&Client->StorageLock);
};

char *Fau3MessagesBufferPop(Fau3ConnectedClient *Client) {
    pthread_mutex_lock(&Client->StorageLock);
    if (Client->Fau3ClientMessages.TotalCount == 0) {
        pthread_mutex_unlock(&Client->StorageLock);
        return NULL;
    };
    char *OutgoingSpeexMessage = Client->Fau3ClientMessages.Buffer[Client->Fau3ClientMessages.Tail];
    Client->Fau3ClientMessages.Tail = (Client->Fau3ClientMessages.Tail + 1) % FAU3_CLIENT_WAIT_MESSAGES_COUNT;
    Client->Fau3ClientMessages.TotalCount--;
    pthread_mutex_unlock(&Client->StorageLock);
    return OutgoingSpeexMessage;
};

bool Fau3PrepareClientOutgoingJson(Fau3ConnectedClient *Client){
    Client->OutgoingJsonObject = json_object_new_object();
    if (Client->OutgoingJsonObject == NULL){
        printf("[ERR]: Could not allocate new json object for client: id=%s", Client->ClientId);
        return false;
    };
    return true;
};

void Fau3SendClientOutgoingJson(Fau3ConnectedClient *Client, char *MessageUuid, bool UseCache){
    const char *OutgoingJsonObjectString = json_object_to_json_string(Client->OutgoingJsonObject);
    if (OutgoingJsonObjectString) {
        Client->OutgoingBufferBytesCount = LWS_PRE + strlen(OutgoingJsonObjectString);
        Client->OutgoingBuffer = malloc(Client->OutgoingBufferBytesCount + 1);
        strcpy(Client->OutgoingBuffer+LWS_PRE, OutgoingJsonObjectString);
        if (lws_send_pipe_choked(Client->Websocket) == 0){
            lws_write( Client->Websocket, (unsigned char *)&Client->OutgoingBuffer[LWS_PRE],
                       Client->OutgoingBufferBytesCount - LWS_PRE, LWS_WRITE_TEXT );
        }else{
            printf("[ERR]: Propably clientId=%s websocket is dead..\n",Client->ClientId);
            lws_set_timeout(Client->Websocket, PENDING_TIMEOUT_AWAITING_PROXY_RESPONSE, LWS_TO_KILL_SYNC);
        };
        free(Client->OutgoingBuffer);
    };
    if (UseCache){
        Fau3StoreOutgoingMessageCache(Client, OutgoingJsonObjectString, MessageUuid);
    };
    json_object_put(Client->OutgoingJsonObject);
};

void Fau3SendClientStream(Fau3ActiveClients *ActiveClients, Fau3ConnectedClient *Client){
    Client->OutgoingBufferBytesCount = LWS_PRE + Client->IncomingBufferBytesCount;
    Client->OutgoingBuffer = malloc(Client->OutgoingBufferBytesCount + 1);
    memcpy(Client->OutgoingBuffer+LWS_PRE, Client->IncomingBuffer, Client->IncomingBufferBytesCount);
    pthread_mutex_lock(&ActiveClients->ClientsLock);
    for (int Idx=0; Idx < ActiveClients->TotalCount; Idx++){
        if (strcmp(ActiveClients->Clients[Idx]->ClientId, Client->ClientId) != 0 && strcmp(ActiveClients->Clients[Idx]->CurrentRoom, Client->CurrentRoom) == 0){
            if (lws_send_pipe_choked(Client->Websocket) == 0){
                lws_write( ActiveClients->Clients[Idx]->Websocket, (unsigned char *)&Client->OutgoingBuffer[LWS_PRE],
                        Client->OutgoingBufferBytesCount - LWS_PRE, LWS_WRITE_TEXT );
            };
        };
    };
    pthread_mutex_unlock(&ActiveClients->ClientsLock);
};

void Fau3SendCollectedPlaylist(Fau3ActiveClients *ActiveClients, struct lws *WebsocketInstance){
    Fau3ConnectedClient *Client = Fau3GetClientPtr(ActiveClients,WebsocketInstance,NULL);
    if (Client){
        pthread_mutex_lock(&Client->InternalLock);
        long PlaylistMessagesCount = Client->Fau3ClientMessages.TotalCount;
        if (PlaylistMessagesCount == 0){
            pthread_mutex_unlock(&Client->InternalLock);
            return;
        };
        char **PayloadArray = malloc(sizeof(char*) * PlaylistMessagesCount);
        for (long Idx =0; Idx < PlaylistMessagesCount; Idx++){
            PayloadArray[Idx] = Fau3MessagesBufferPop(Client);
        };
        Fau3Playlist(Client, PayloadArray, PlaylistMessagesCount);
        for (long Idx =0; Idx < PlaylistMessagesCount; Idx++){
            free(PayloadArray[Idx]);
        };
        free(PayloadArray);
        pthread_mutex_unlock(&Client->InternalLock);
        printf("[OK]:[%s]: Record sended to client\n",Client->ClientId);
    };
};

void Fau3DeleteOutgoingMessageCache(Fau3ConnectedClient *Client, char *MessageUuid){
    char *Object = GetStorageRecord(Client, MessageUuid, true);
    if (Object){
        free(Object);
    };
    for (short Idx = 0; Idx < Client->CachedMessages.Count; Idx++){
        if (strcmp(Client->CachedMessages.MessagesMetaData[Idx].MessageUuid, MessageUuid) == 0){
            if (Idx < Client->CachedMessages.Count-1){
                memmove(&Client->CachedMessages.MessagesMetaData[Idx],
                        &Client->CachedMessages.MessagesMetaData[Idx+1],
                        sizeof(Fau3SingleCacheMeta) * (Client->CachedMessages.Count - Idx - 1));
            };
            printf("[OK][MessageCache]: Message removed from cache: %s\n", MessageUuid);
            Client->CachedMessages.Count--;
            break;
        };
    };
};

void Fau3StoreOutgoingMessageCache(Fau3ConnectedClient *Client, char *OutgoingJsonObjectString, char *MessageUuid){
    if (Client->CachedMessages.Count == FAU3_OUTGOING_MSG_CACHE_SIZE){
        char *OldestRecordUuid = Client->CachedMessages.MessagesMetaData[0].MessageUuid;
        char *Object = GetStorageRecord(Client, OldestRecordUuid, true);
        if (Object){
            free(Object);
        };
        memmove(&Client->CachedMessages.MessagesMetaData[0],
                &Client->CachedMessages.MessagesMetaData[1],
                sizeof(Fau3SingleCacheMeta) * (FAU3_OUTGOING_MSG_CACHE_SIZE - 1));
        Client->CachedMessages.Count--;
    };
    AddStorageRecord(Client, MessageUuid, OutgoingJsonObjectString,FAU3_OUTGOING_MSG_RESEND_TIMEOUT*2);
    bool IsCachedAlready = false;
    for (short Idx = 0; Idx < Client->CachedMessages.Count; Idx++){
        if (strcmp(Client->CachedMessages.MessagesMetaData[Idx].MessageUuid, MessageUuid) == 0){
            Client->CachedMessages.MessagesMetaData[Idx].RetryCount++;
            Client->CachedMessages.MessagesMetaData[Idx].ResendCouter = 0;
            IsCachedAlready = true;
            break;
        };
    };
    if (IsCachedAlready == false){
        printf("[OK][MessageCache]: New message added to cache: %s\n", MessageUuid);
        int CurrentCacheIndex = Client->CachedMessages.Count;
        Client->CachedMessages.MessagesMetaData[CurrentCacheIndex].ResendCouter = 0;
        Client->CachedMessages.MessagesMetaData[CurrentCacheIndex].RetryCount = 0;
        memcpy(Client->CachedMessages.MessagesMetaData[CurrentCacheIndex].MessageUuid, MessageUuid, 37);
        Client->CachedMessages.Count++;
    };
};

void *Fau3CheckOutgoingMessageCache(void *_Client){
    Fau3ConnectedClient *Client = (Fau3ConnectedClient *)_Client;
    Client->IsCacheCheckerRunning = true;
    struct json_tokener *JsonTokener = json_tokener_new();
    while (Client->IsCacheCheckerRunning) {
        pthread_mutex_lock(&Client->InternalLock);
        for (short Idx =0; Idx < Client->CachedMessages.Count; Idx++){
            if (Client->CachedMessages.MessagesMetaData[Idx].ResendCouter >= FAU3_OUTGOING_MSG_RESEND_TIMEOUT){
                char *MessageUuid = Client->CachedMessages.MessagesMetaData[Idx].MessageUuid;
                if (Client->CachedMessages.MessagesMetaData[Idx].RetryCount < FAU3_OUTGOING_MSG_RETRY_COUNT){
                    char *OutgoingJsonObject = GetStorageRecord(Client, MessageUuid, false);
                    if (OutgoingJsonObject){
                        Client->OutgoingJsonObject = json_tokener_parse_ex(JsonTokener,
                                                                        OutgoingJsonObject,strlen(OutgoingJsonObject));
                        json_tokener_reset(JsonTokener);
                        Fau3SendClientOutgoingJson(Client, MessageUuid, true);
                        printf("[OK][MessageCache]: Resending cached message: %s\n", MessageUuid);
                        free(OutgoingJsonObject);
                    } else {
                        Client->CachedMessages.MessagesMetaData[Idx].RetryCount++;
                    };
                } else {
                    Fau3DeleteOutgoingMessageCache(Client, MessageUuid);
                    printf("[OK][MessageCache]: FAU3_OUTGOING_MSG_RETRY_COUNT limid reached for message: %s\n", MessageUuid);
                };
            } else {
                Client->CachedMessages.MessagesMetaData[Idx].ResendCouter++;
            };
        };
        pthread_mutex_unlock(&Client->InternalLock);
        sleep(1);
    };
    json_tokener_free(JsonTokener);
    return;
};












