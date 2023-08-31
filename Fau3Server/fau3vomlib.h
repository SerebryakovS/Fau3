
#include            <time.h>
#include            <json.h>
#include           <stdio.h>
#include          <stdlib.h>
#include          <string.h>
#include          <signal.h>
#include         <stdbool.h>
#include         <pthread.h>
#include       <uuid/uuid.h>
#include   <libwebsockets.h>
#include <hiredis/hiredis.h>

#define FAU3_BUFFER_BYTES               4096
#define FAU3_CLIENT_ROUTER_WS_PORT      8901
#define FAU3_ROUTER_ROUTER_WS_PORT      8902
#define FAU3_CLIENTS_COUNT               256
#define FAU3_CLIENT_WAIT_MESSAGES_COUNT    5
#define FAU3_ECHO_ENABLED                  0
#define FAU3_EXIT_ON_EMPTY_PONG            0

#define FAU3_IFACE_PIPE_NAME     "Fau3Iface"

#define FAU3_OUTGOING_MSG_CACHE_SIZE       5
#define FAU3_OUTGOING_MSG_RESEND_TIMEOUT   5
#define FAU3_OUTGOING_MSG_RETRY_COUNT      3

#define FAU3_STREAM_BEGIN_MARK        "F3SB" // should be extended in future and be like header with additional information about codec,length,etc.
#define FAU3_STREAM_END_MARK          "F3FN"

#define REDIS_HOSTNAME           "127.0.0.1"
#define REDIS_PORT                      6379

enum FAU3_STATUS_CODE {
    FAU3_OK      =  0,
    FAU3_PONG    =  1,
    FAU3_MONED   =  2,
    FAU3_STORED  =  3,
    FAU3_INVALID = -1,
    FAU3_SERROR  = -2,
};

typedef struct Fau3ClientMessagesToRead{
    char *Buffer[FAU3_CLIENT_WAIT_MESSAGES_COUNT];
    int                                      Head;
    int                                      Tail;
    int                                TotalCount;
} Fau3ClientMessagesToRead;


typedef struct Fau3SingleCacheMeta{
    char MessageUuid[37];
    int     ResendCouter;
    int       RetryCount;
}Fau3SingleCacheMeta;

typedef struct Fau3ClientCache{
    Fau3SingleCacheMeta MessagesMetaData[FAU3_OUTGOING_MSG_CACHE_SIZE];
    int                                                          Count;
}Fau3ClientCache;

typedef struct PeriodicMessage{
    int      Timeout;
    bool    SendFlag;
    int      Counter;
}PeriodicMessage;

typedef struct Fau3ConnectedClient {
    redisContext                    *RStorageIO;
    redisContext                   *RStorageSub;
    bool                            Reconnected;
    char                           ClientId[18];
    char                        CurrentRoom[30];
    pthread_t                CacheCheckerThread;
    pthread_t        RStorageSubscriptionThread;
    pthread_mutex_t                 StorageLock;
    pthread_mutex_t                InternalLock;
    struct lws                       *Websocket;
    int                IncomingBufferBytesCount;
    int                OutgoingBufferBytesCount;
    char                        *IncomingBuffer;
    char                        *OutgoingBuffer;
    time_t                        ConnectedTime;
    Fau3ClientCache              CachedMessages;
    bool                      RStorageSubSignal;
    bool                       IsOutgoingStream;
    bool                  IsCacheCheckerRunning;
    struct json_tokener            *JsonTokener;
    json_object             *OutgoingJsonObject;
    json_object             *IncomingJsonObject;
    Fau3ClientMessagesToRead Fau3ClientMessages;
    PeriodicMessage                PeriodicPing;
    PeriodicMessage             PeriodicMonitor;
}Fau3ConnectedClient;

typedef struct Fau3ActiveClients{
    Fau3ConnectedClient *Clients[FAU3_CLIENTS_COUNT];
    pthread_mutex_t                      ClientsLock;
    long                                  TotalCount;
} Fau3ActiveClients;

void Fau3HandleRequest(Fau3ConnectedClient *Client);
bool Fau3HandleStream(Fau3ActiveClients *ActiveClients, Fau3ConnectedClient *Client);
void GenerateUuid(char *Uuid);
////////////////////////////////FAU3VOM(D)
void Fau3Ping(Fau3ConnectedClient *Client);
void Fau3Propogate(Fau3ConnectedClient *Client, char *RoomId, char *Payload);
void Fau3Subscribe(Fau3ConnectedClient *Client, char *RoomId);
void Fau3Playlist(Fau3ConnectedClient *Client, char **PayloadArray, long PayloadCount);
void Fau3Confirm(Fau3ConnectedClient *Client, char *Uuid, int Status);
void Fau3Monitor(Fau3ActiveClients *ActiveClients, Fau3ConnectedClient *Client);
////////////////////////////////ObjectsStorage
bool OpenObjectsStorage(Fau3ConnectedClient *Client);
void CloseObjectsStorage(Fau3ConnectedClient *Client);
void AddStorageRecord(Fau3ConnectedClient *Client, char *ObjectUuid, char *Object, int Timeout);
char *GetStorageRecord(Fau3ConnectedClient *Client, char *ObjectUuid, bool DeleteAfter);
char **FindObjectsKeysByClientId(Fau3ConnectedClient *Client, const char *ClientId, long *ObjectsCount);
void PublishMessageToStorage(Fau3ConnectedClient *Client, char *Object);
void *SubscribeStorageMessages(void *Client);
void StopStorageSubscription(Fau3ConnectedClient *Client);
////////////////////////////////ClientsHandler
bool Fau3AppendClient(Fau3ActiveClients *ActiveClients, struct lws *WebsocketInstance, char *ClientId);
bool Fau3DeleteClient(Fau3ActiveClients *ActiveClients, struct lws *WebsocketInstance);
void Fau3GetSubscriptionRequestsFlags(Fau3ActiveClients *ActiveClients, struct lws *WebsocketInstance);
void Fau3SetSubscriptionRequestsFlags(Fau3ActiveClients *ActiveClients, struct lws *WebsocketInstance);
void Fau3SendCollectedPlaylist(Fau3ActiveClients *ActiveClients, struct lws *WebsocketInstance);
Fau3ConnectedClient *Fau3GetClientPtr(Fau3ActiveClients *ActiveClients, struct lws *WebsocketInstance, long *SearchingIndex);
Fau3ConnectedClient *Fau3GetClientPtrById(Fau3ActiveClients *ActiveClients, char* ClientId, long *SearchingIndex);
bool Fau3ClientIncomingBufferAppend(Fau3ActiveClients *ActiveClients, struct lws *WebsocketInstance, unsigned char *IncomingBuffer, long Length);
void Fau3MessagesBufferPush(Fau3ConnectedClient *Client, char *IncomingSpeexMessage);
char *Fau3MessagesBufferPop(Fau3ConnectedClient *Client);
bool Fau3PrepareClientOutgoingJson(Fau3ConnectedClient *Client);
void Fau3SendClientOutgoingJson(Fau3ConnectedClient *Client, char *MessageUuid, bool UseCache);
void Fau3SendClientStream(Fau3ActiveClients *ActiveClients, Fau3ConnectedClient *Client);

void Fau3DeleteOutgoingMessageCache(Fau3ConnectedClient *Client, char *MessageUuid);
void Fau3StoreOutgoingMessageCache(Fau3ConnectedClient *Client, char *OutgoingJsonObjectString, char *MessageUuid);
void *Fau3CheckOutgoingMessageCache(void *_Client);
