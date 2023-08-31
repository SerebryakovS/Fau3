
#include "fau3vomlib.h"


unsigned char *Fau3Response(const char *Uuid, bool IsSuccessfull){
    json_object *OutgoingJsonObject = json_object_new_object();
    json_object_object_add(OutgoingJsonObject, "MessageUuid", json_object_new_string(Uuid));
    json_object_object_add(OutgoingJsonObject, "Status", json_object_new_boolean(IsSuccessfull));
    return (unsigned char *)json_object_to_json_string_ext(OutgoingJsonObject, JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY);
};

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

unsigned char *HandleFau3Request(unsigned char *IncomingPacket){
    json_object *IncomingJsonObject = json_tokener_parse((const char *)IncomingPacket);
    json_object *FieldObject = NULL;
    ////////////////////////////////////////////////////////////////////////////
    json_object_object_get_ex(IncomingJsonObject, "MessageType", &FieldObject);
    const char* IncomingMessageType = json_object_get_string(FieldObject);
    json_object_object_get_ex(IncomingJsonObject, "MessageUuid", &FieldObject);
    const char* IncomingMessageUuid = json_object_get_string(FieldObject);
    ////////////////////////////////////////////////////////////////////////////
    if (strcmp(IncomingMessageType, "Ping") == 0) {
        AddStorageRecord(IncomingMessageUuid, IncomingPacket);
        SetStorageObjectTimeout(IncomingMessageUuid, 3);
        GetStorageRecord(IncomingMessageUuid);
        return Fau3Response(IncomingMessageUuid, true);
    } else {
        printf("[ERR]: Unsupported message type\n");
        return NULL;
    };
};
