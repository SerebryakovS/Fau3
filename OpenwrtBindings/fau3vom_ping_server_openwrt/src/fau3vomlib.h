
#include            <time.h>
#include            <json.h>
#include           <stdio.h>
#include          <stdlib.h>
#include          <string.h>
#include         <stdbool.h>
#include       <uuid/uuid.h>
#include <hiredis/hiredis.h>

#define FAU3_BUFFER_BYTES               4096
#define FAU3_WS_PORT_CLIENT_ROUTER      8901
#define FAU3_WS_PORT_ROUTER_SERVER      8902


unsigned char *HandleFau3Request(unsigned char *IncomingPacket);
unsigned char *Fau3Response(const char *Uuid, bool IsSuccessfull);
void GenerateUuid(char *Uuid);
unsigned char *Fau3PingServerMessage();

////////////////

bool OpenObjectsStorage();
void CloseObjectsStorage();
bool AddStorageRecord(const char *ObjectUuid, unsigned char *SpeexPayload);
bool GetStorageRecord(const char *ObjectUuid);
bool SetStorageObjectTimeout(const char *ObjectUuid, int Timeout);
