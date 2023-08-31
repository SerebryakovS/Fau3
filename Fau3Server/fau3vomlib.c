
#include "fau3vomlib.h"

void GenerateUuid(char *UuidBuffer){
    time_t TimeObject;
    srand((unsigned) time(&TimeObject));
    char Characters[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
    for(int Idx = 0; Idx < 36; ++Idx) {
        UuidBuffer[Idx] = Characters[rand()%16];
    };
    UuidBuffer[8]  =  '-';
    UuidBuffer[13] =  '-';
    UuidBuffer[18] =  '-';
    UuidBuffer[23] =  '-';
    UuidBuffer[36] = '\0';
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

void Fau3Confirm(Fau3ConnectedClient *Client, char *Uuid, int Status){
    if (Fau3PrepareClientOutgoingJson(Client)){
        json_object_object_add(Client->OutgoingJsonObject, "MessageType", json_object_new_string("Confirm"));
        json_object_object_add(Client->OutgoingJsonObject, "MessageUuid", json_object_new_string(Uuid));
        json_object_object_add(Client->OutgoingJsonObject, "Status", json_object_new_int(Status));
        Fau3SendClientOutgoingJson(Client, Uuid, false);
    };
};

void Fau3Ping(Fau3ConnectedClient *Client){
    char Uuid[37]; GenerateUuid(Uuid);
    if (Fau3PrepareClientOutgoingJson(Client)){
        json_object_object_add(Client->OutgoingJsonObject, "MessageType", json_object_new_string("Ping"));
        json_object_object_add(Client->OutgoingJsonObject, "MessageUuid", json_object_new_string(Uuid));
        Fau3SendClientOutgoingJson(Client, Uuid, false);
    };
};

void Fau3Propogate(Fau3ConnectedClient *Client, char *RoomId, char *Payload) {
    char Uuid[37]; GenerateUuid(Uuid);
    if (Fau3PrepareClientOutgoingJson(Client)){
        json_object_object_add(Client->OutgoingJsonObject, "MessageType", json_object_new_string("Propogate"));
        json_object_object_add(Client->OutgoingJsonObject, "MessageUuid", json_object_new_string(Uuid));
        json_object_object_add(Client->OutgoingJsonObject, "RoomId", json_object_new_string(RoomId));
        json_object_object_add(Client->OutgoingJsonObject, "Payload", json_object_new_string(Payload));
        Fau3SendClientOutgoingJson(Client, Uuid, true);
    };
};

void Fau3Subscribe(Fau3ConnectedClient *Client, char *RoomId){
    char Uuid[37]; GenerateUuid(Uuid);
    if (Fau3PrepareClientOutgoingJson(Client)){
        json_object_object_add(Client->OutgoingJsonObject, "MessageType", json_object_new_string("Subscribe"));
        json_object_object_add(Client->OutgoingJsonObject, "MessageUuid", json_object_new_string(Uuid));
        json_object_object_add(Client->OutgoingJsonObject, "RoomId"     , json_object_new_string(RoomId));
        Fau3SendClientOutgoingJson(Client, Uuid, true);
    };
};

void Fau3Monitor(Fau3ActiveClients *ActiveClients, Fau3ConnectedClient *Client){
    char Uuid[37]; GenerateUuid(Uuid);
    if (Fau3PrepareClientOutgoingJson(Client)){
        json_object_object_add(Client->OutgoingJsonObject, "MessageType", json_object_new_string("Monitor"));
        json_object_object_add(Client->OutgoingJsonObject, "MessageUuid", json_object_new_string(Uuid));
        struct json_object *PayloadJsonArray = json_object_new_array();
        time_t CurrentTime; time(&CurrentTime);
        for (int Idx = 0; Idx < ActiveClients->TotalCount; Idx++) {
            Fau3ConnectedClient *Client = ActiveClients->Clients[Idx];
            struct json_object *ClientJsonObject = json_object_new_object();
            json_object_object_add(ClientJsonObject, "ClientId", json_object_new_string(Client->ClientId));
            json_object_object_add(ClientJsonObject, "RoomId", json_object_new_string(Client->CurrentRoom));
            json_object_object_add(ClientJsonObject, "AliveTime", json_object_new_double(difftime(CurrentTime,Client->ConnectedTime)));

            json_object_array_add(PayloadJsonArray, ClientJsonObject);
        };
        json_object_object_add(Client->OutgoingJsonObject, "ConnectedClients", PayloadJsonArray);
        Fau3SendClientOutgoingJson(Client, Uuid, true);
    };
};

void Fau3Playlist(Fau3ConnectedClient *Client, char **PayloadArray, long PayloadCount){
    char Uuid[37]; GenerateUuid(Uuid);
    if (Fau3PrepareClientOutgoingJson(Client)){
        json_object_object_add(Client->OutgoingJsonObject, "MessageType", json_object_new_string("Playlist"));
        json_object_object_add(Client->OutgoingJsonObject, "MessageUuid", json_object_new_string(Uuid));
        struct json_object *PayloadJsonArray = json_object_new_array();
        for (int Idx = 0; Idx < PayloadCount; Idx++) {
            struct json_object *SinglePayloadJsonObject = json_tokener_parse(PayloadArray[Idx]);
            json_object_array_add(PayloadJsonArray, SinglePayloadJsonObject);
        };
        json_object_object_add(Client->OutgoingJsonObject, "Records", PayloadJsonArray);
        Fau3SendClientOutgoingJson(Client, Uuid, true);
    };
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

void Fau3HandleRequest(Fau3ConnectedClient *Client){
    short ConfirmStatus = FAU3_OK;
    Client->IsOutgoingStream = false;
    json_object *FieldObject = NULL;
    json_object_object_get_ex(Client->IncomingJsonObject, "MessageType", &FieldObject);
    const char* IncomingMessageType = json_object_get_string(FieldObject);
    json_object_object_get_ex(Client->IncomingJsonObject, "MessageUuid", &FieldObject);
    const char* IncomingMessageUuid = json_object_get_string(FieldObject);
    char ConfirmUuid[37];
    memcpy(ConfirmUuid, IncomingMessageUuid, 37);
    if  (strcmp(IncomingMessageType, "Confirm") == 0) {
        json_object_object_get_ex(Client->IncomingJsonObject, "Status", &FieldObject);
        int Status = json_object_get_int(FieldObject);
        switch (Status){
            case FAU3_PONG:
                Client->PeriodicPing.SendFlag = false;
                break;
            case FAU3_OK:
            case FAU3_STORED:
                Fau3DeleteOutgoingMessageCache(Client, (char *)IncomingMessageUuid);
            default:
                break;
        };
    } else {
        ///////////////////////////////////////////////
        if (strcmp(IncomingMessageType, "Ping") == 0) {
            ConfirmStatus = FAU3_PONG;
        ///////////////////////////////////////////////////////////
        } else if (strcmp(IncomingMessageType, "Propogate") == 0) {
            json_object_object_get_ex(Client->IncomingJsonObject, "RoomId", &FieldObject);
            const char* RoomId = json_object_get_string(FieldObject);
            strcpy(Client->CurrentRoom, (char *)RoomId);
            json_object_object_del(Client->IncomingJsonObject, "MessageType");
            json_object_object_del(Client->IncomingJsonObject, "MessageUuid");
            json_object_object_del(Client->IncomingJsonObject, "RoomId");
            json_object_object_add(Client->IncomingJsonObject, "ClientId", json_object_new_string(Client->ClientId));
            const char *JsonObjextStringToPublish = json_object_to_json_string(Client->IncomingJsonObject);
            PublishMessageToStorage(Client,(char *)JsonObjextStringToPublish);
            ConfirmStatus = FAU3_STORED;
        ///////////////////////////////////////////////////////////
        } else if (strcmp(IncomingMessageType, "Subscribe") == 0) {
            json_object_object_get_ex(Client->IncomingJsonObject, "RoomId", &FieldObject);
            const char* RoomId = json_object_get_string(FieldObject);
            StopStorageSubscription(Client);
            if (pthread_create(&Client->RStorageSubscriptionThread,
                NULL, SubscribeStorageMessages, Client) != 0){
                printf("[ERR]: Could not create new Subscription thread for: %s\n",Client->ClientId);
                ConfirmStatus = FAU3_SERROR;
            };
            strcpy(Client->CurrentRoom, (char *)RoomId);
            if (json_object_object_get_ex(Client->IncomingJsonObject, "IsMonitorRequired", &FieldObject)){
                int MonitorTimeout = json_object_get_int(FieldObject);
                Client->PeriodicMonitor.Timeout = MonitorTimeout;
                Client->PeriodicMonitor.Counter = MonitorTimeout;
            };
            if (json_object_object_get_ex(Client->IncomingJsonObject, "IsPingRequired", &FieldObject)){
                int PingTimeout = json_object_get_int(FieldObject);
                Client->PeriodicPing.Timeout = PingTimeout;
                Client->PeriodicPing.Counter = PingTimeout;
            };
            ConfirmStatus = FAU3_OK;
        //////////////////////////////////////////////////////////
        } else {
            printf("[ERR]: Unsupported message type for Fau3Server\n");
            ConfirmStatus = FAU3_SERROR;
        };
        Fau3Confirm(Client, ConfirmUuid, ConfirmStatus);
    };
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

bool Fau3HandleStream(Fau3ActiveClients *ActiveClients, Fau3ConnectedClient *Client){
    short StreamStartMarkLength = sizeof(FAU3_STREAM_BEGIN_MARK) - 1;
    char StreamStartMark[sizeof(FAU3_STREAM_BEGIN_MARK)];
    memcpy(StreamStartMark,&Client->IncomingBuffer[0],StreamStartMarkLength);
    //-----------------------------------------------------------------
    short StreamStopMarkLength = sizeof(FAU3_STREAM_END_MARK);
    char StreamEndMark[sizeof(FAU3_STREAM_END_MARK)];
    memcpy(StreamEndMark,&Client->IncomingBuffer[Client->IncomingBufferBytesCount-StreamStartMarkLength], StreamStopMarkLength);
    //-----------------------------------------------------------------
    if (strcmp(StreamStartMark,FAU3_STREAM_BEGIN_MARK) == 0 && strcmp(StreamEndMark,FAU3_STREAM_END_MARK) == 0){
        Fau3SendClientStream(ActiveClients, Client);
        return true;
    };
    return false; // this not stream packet
};



