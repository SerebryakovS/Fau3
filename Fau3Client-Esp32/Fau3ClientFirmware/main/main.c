
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "audio_element.h"
#include "audio_pipeline.h"
#include "audio_event_iface.h"
#include "audio_mem.h"
#include "audio_common.h"
#include "i2s_stream.h"
#include "stdbool.h"
#include "mp3_decoder.h"
#include "esp_peripherals.h"
#include "periph_touch.h"
#include "periph_adc_button.h"
#include "periph_button.h"
#include "board.h"
#include "math.h"
#include "rom/ets_sys.h"
#include "uuid.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <inttypes.h>
#include <stdlib.h>
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_wnm.h"
#include "esp_rrm.h"
#include "esp_mbo.h"
#include "esp_event.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_websocket_client.h"
#include "esp_transport.h"
#include "esp_transport_tcp.h"
#include "esp_heap_caps.h"
#include <cJSON.h>
#include "mbedtls/base64.h"
#include <speex/speex.h>
// calculate the size of 'output' buffer required for a 'input' buffer of length x during Base64 encoding operation
#define B64ENCODE_OUT_SAFESIZE(x) ((((x) + 3 - 1)/3) * 4 + 1)
// calculate the size of 'output' buffer required for a 'input' buffer of length x during Base64 decoding operation
#define B64DECODE_OUT_SAFESIZE(x) (((x)*3)/4)

static const char *TAG = "FAU2OS";

#define AudioSampleRate      8000
#define AudioBitsPerSample     16
#define AudioChannelsCount      1
#define SpeexInFrameSize    160*2
#define SpeexOutFrameSize      38
#define SpeexQualityLevel       8 // 15 [kbps]

#define MinimumRecordBlobSize 100  // 20 [msec] x 100  = 2  [sec]
#define MaximumRecordBlobSize 1000 // 20 [msec] x 1000 = 20 [sec]

///////////////////////////////////////////////////////////////////////////

#define CACHE_TIMEOUT_MS 5000

typedef struct CacheNode {
    char          Uuid[37];
    uint32_t     Timestamp;
    struct CacheNode* Next;
} CacheNode;

static CacheNode* CacheHead = NULL;

void AddUuidToCache(const char* Uuid) {
    CacheNode* NewNode = (CacheNode*)malloc(sizeof(CacheNode));
    if (NewNode) {
        strcpy(NewNode->Uuid, Uuid);
        NewNode->Timestamp = esp_log_timestamp();
        NewNode->Next = CacheHead;
        CacheHead = NewNode;
    };
};

bool IsUuidCached(const char* Uuid) {
    CacheNode* CurrentNode = CacheHead;
    while (CurrentNode != NULL) {
        if (strcmp(CurrentNode->Uuid, Uuid) == 0) {
            return true;  // Found a duplicate UUID in the cache
        };
        CurrentNode = CurrentNode->Next;
    };
    return false;  // UUID not found in the cache
};

void RemoveExpiredUuidEntries() {
    uint32_t CurrentTime = esp_log_timestamp();
    CacheNode** CurrentNodePtr = &CacheHead;
    while (*CurrentNodePtr != NULL) {
        if ((CurrentTime - (*CurrentNodePtr)->Timestamp) >= CACHE_TIMEOUT_MS) {
            CacheNode* Temp = *CurrentNodePtr;
            *CurrentNodePtr = (*CurrentNodePtr)->Next;
            free(Temp);
        } else {
            CurrentNodePtr = &(*CurrentNodePtr)->Next;
        };
    };
};

///////////////////////////////////////////////////////////////////////////



typedef struct {
    char  UUID[37];
    int  NumChunks;
    int  TotalSize;
    int  ReadIndex;
} SingleRecordMeta;

SingleRecordMeta RecordingBlob;


// #define TangentaStatusLed 34
// TaskHandle_t TangentaStatusLedHandle;

// void SetStatusLedNoSignal(void *Arg){
//     for ( short Idx = 0; Idx < 3; Idx++ ){
//         gpio_set_level(TangentaStatusLed,1);
//         vTaskDelay(pdMS_TO_TICKS(500));
//         gpio_set_level(TangentaStatusLed,0);
//         vTaskDelay(pdMS_TO_TICKS(500));
//     };
//     vTaskDelete(NULL);
// };
//
// void SetStatusLedConnectd(void *Arg){
//     for ( short Idx = 0; Idx < 2; Idx++ ){
//         gpio_set_level(TangentaStatusLed,1);
//         vTaskDelay(pdMS_TO_TICKS(1000));
//         gpio_set_level(TangentaStatusLed,0);
//         vTaskDelay(pdMS_TO_TICKS(1000));
//     };
//     vTaskDelete(NULL);
// };


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

nvs_handle_t NVS_Handle;
SemaphoreHandle_t NVSMutex;

bool NVSWrite(unsigned char *IncomingBlob, size_t BlobSize, char *Key){
    bool ReturnValue = false;
    if (xSemaphoreTake(NVSMutex, portMAX_DELAY)){
        esp_err_t Err = nvs_set_blob(NVS_Handle, Key, IncomingBlob, BlobSize);
        if (Err != ESP_OK){
            ESP_LOGE(TAG, "[NVSWrite]: Could not SET blob: %s", (char *)esp_err_to_name(Err));
        } else {
            nvs_commit(NVS_Handle);
            ReturnValue = true;
        };
        xSemaphoreGive(NVSMutex);
    };
    return ReturnValue;
};

bool NVSReadAndErase(unsigned char *OutgoingBlob, size_t BlobSize, char *Key){
    bool ReturnValue = false;
    esp_err_t Err;
    if (xSemaphoreTake(NVSMutex, portMAX_DELAY)){
        if (OutgoingBlob != NULL){
            Err = nvs_get_blob(NVS_Handle, Key, OutgoingBlob, &BlobSize);
            if (Err != ESP_OK){
                ESP_LOGE(TAG, "[NVSReadAndErase]: Could not GET record chunk: %s -> %s", (char *)esp_err_to_name(Err), Key);
            } else {
                ReturnValue = true;
            };
        };
        nvs_erase_key(NVS_Handle, Key);
        nvs_commit(NVS_Handle);
        xSemaphoreGive(NVSMutex);
    };
    return ReturnValue;
};
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

unsigned short SelectedLine;

bool IsStreamModeActivated = false;

typedef struct{
    char                               *StreamData;
    size_t                              BufferSize;
    SemaphoreHandle_t                ReadWriteSema;
    bool                         IsReceivingStream;
    struct timeval     LastReceivedTime;
} StreamInfo;

StreamInfo Stream = {
    .StreamData        = NULL,
    .IsReceivingStream = false,
    .BufferSize        = 0,
};

typedef struct{
    SingleRecordMeta Records[CONFIG_PLAYLIST_SIZE];
    SingleRecordMeta                *PlayingRecord;
    SemaphoreHandle_t      PlaylistUpdateSemaphore;
    int                               CountRecords;
} PlaylistInfo;

PlaylistInfo Playlist = {
    .PlayingRecord = NULL,
    .CountRecords  = 0
};

void AppendPlaylistRecord(unsigned char *RecordPayload, size_t RecordSize){
    if(xSemaphoreTake(Playlist.PlaylistUpdateSemaphore, portMAX_DELAY)){
        if (Playlist.CountRecords == CONFIG_PLAYLIST_SIZE) {
            SingleRecordMeta *OldestRecord = &Playlist.Records[0];
            for (int Idx = 0; Idx < OldestRecord->NumChunks; Idx++) {
                char ChunkKey[50];
                sprintf(ChunkKey, "%s_%d", OldestRecord->UUID, Idx);
                NVSReadAndErase(NULL, 0, ChunkKey);
            };
            memmove(&Playlist.Records[0], &Playlist.Records[1],
                    sizeof(SingleRecordMeta) * (CONFIG_PLAYLIST_SIZE - 1));
            Playlist.CountRecords--;
        };
        SingleRecordMeta NewPlayerRecord;
        GenerateUUIDv4(NewPlayerRecord.UUID);
        NewPlayerRecord.NumChunks  = RecordSize / SpeexOutFrameSize;
        NewPlayerRecord.TotalSize  = RecordSize;
        NewPlayerRecord.ReadIndex = 0;

        GenerateUUIDv4(NewPlayerRecord.UUID);
        memcpy(&Playlist.Records[Playlist.CountRecords], &NewPlayerRecord, sizeof(SingleRecordMeta));
        int Offset = 0;
        for (int Idx = 0; Idx < NewPlayerRecord.NumChunks; Idx++){
            char ChunkKey[50];
            sprintf(ChunkKey, "%s_%d", NewPlayerRecord.UUID+25, Idx);
            if (NVSWrite(RecordPayload + Offset, SpeexOutFrameSize, ChunkKey) == true){
                Offset += SpeexOutFrameSize;
            } else {
                Idx--;
            };
        };
        ESP_LOGI(TAG,"[OK]: New record appened to playlist with index=%d, UUID=%s and size:%d",Playlist.CountRecords,NewPlayerRecord.UUID,RecordSize);
        Playlist.CountRecords++;
        xSemaphoreGive(Playlist.PlaylistUpdateSemaphore);
    };
};

void AppendStreamChunk(unsigned char *DataChunk){
    if(xSemaphoreTake(Stream.ReadWriteSema, portMAX_DELAY)){
        if (Stream.BufferSize == 0){
            Stream.StreamData = malloc(SpeexOutFrameSize);
        }else{
            Stream.StreamData = realloc(Stream.StreamData, Stream.BufferSize+SpeexOutFrameSize);
        };
        memcpy(Stream.StreamData+Stream.BufferSize, DataChunk, SpeexOutFrameSize);
        Stream.BufferSize+=SpeexOutFrameSize;
        gettimeofday(&Stream.LastReceivedTime, NULL);
        Stream.IsReceivingStream = true;
        xSemaphoreGive(Stream.ReadWriteSema);
    };
};

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

static EventGroupHandle_t wifi_event_group;
static esp_netif_t *sta_netif = NULL;

static void WifiEventHandler(void* arg, esp_event_base_t event_base,int32_t event_id, void* event_data){
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG,"WIFI_EVENT_STA_START");
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG,"WIFI_EVENT_STA_DISCONNECTED");
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        ESP_LOGI(TAG,"WIFI_EVENT_STA_CONNECTED");
#if CONFIG_WIFI_RSSI_THRESHOLD
        ESP_LOGI(TAG, "setting rssi threshold as %d\n", CONFIG_WIFI_RSSI_THRESHOLD);
        esp_wifi_set_rssi_threshold(CONFIG_WIFI_RSSI_THRESHOLD);
#endif
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP){
        ESP_LOGI(TAG,"IP_EVENT_STA_GOT_IP");
        esp_netif_ip_info_t ip_info;
        esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"), &ip_info);
        ESP_LOGI(TAG, "Access Point IP Address: %s", ip4addr_ntoa(&ip_info.gw));
    }else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_BSS_RSSI_LOW){
        ESP_LOGI(TAG,"WIFI_EVENT_STA_BSS_RSSI_LOW");
        esp_wifi_disconnect();
    };
};

static void ConfigureWiFi(void){
	ESP_ERROR_CHECK(esp_netif_init());
	wifi_event_group = xEventGroupCreate();
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	sta_netif = esp_netif_create_default_wifi_sta();
	assert(sta_netif);

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
	ESP_ERROR_CHECK( esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &WifiEventHandler, NULL) );
	ESP_ERROR_CHECK( esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &WifiEventHandler, NULL) );
#if CONFIG_WIFI_RSSI_THRESHOLD
	ESP_ERROR_CHECK( esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_BSS_RSSI_LOW, &WifiEventHandler, NULL) );
#endif
	ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
	wifi_config_t wifi_config = {
		.sta = {
			.ssid = CONFIG_WIFI_SSID,
			.password = CONFIG_WIFI_PASSWORD,
			// .rm_enabled =1,
			// .btm_enabled =1,
			.mbo_enabled =1,
			.pmf_cfg.capable = 1,
			// .ft_enabled =1,
		},
	};
	ESP_LOGI(TAG, "Setting WiFi configuration SSID %s..", wifi_config.sta.ssid);
	ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
	ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
	ESP_ERROR_CHECK( esp_wifi_start() );
};

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

esp_websocket_client_handle_t WebSocketClient;
esp_websocket_client_config_t websocket_cfg = {};

char UniqueIdSrt[18];
void SetUniqueIdSrt(){
    uint8_t UniqueId[ 6]; esp_efuse_mac_get_default(UniqueId);
    sprintf(UniqueIdSrt, "%02X:%02X:%02X:%02X:%02X:%02X",
            UniqueId[0], UniqueId[1], UniqueId[2],
            UniqueId[3], UniqueId[4], UniqueId[5]
    );
};


enum FAU3_STATUS_CODE {
    FAU3_OK      =  0,
    FAU3_PONG    =  1,
    FAU3_STORED  =  3,
    FAU3_INVALID = -1,
    FAU3_SERROR  = -2,
};

void Fau3Propogate(SingleRecordMeta *RecordingBlob){
    int PayloadSpeexSize = RecordingBlob->TotalSize;
    ESP_LOGI(TAG,"Propogating new message of size=%d and UUID=%s",PayloadSpeexSize,RecordingBlob->UUID);
    cJSON *PropogateObject = cJSON_CreateObject();
    cJSON_AddStringToObject(PropogateObject, "MessageType", "Propogate");
    cJSON_AddStringToObject(PropogateObject, "MessageUuid", RecordingBlob->UUID);
    char RoomId[20]; sprintf(RoomId,"Channel%d",SelectedLine);
    cJSON_AddStringToObject(PropogateObject, "RoomId", RoomId);
    unsigned char *PayloadSpeex = malloc(PayloadSpeexSize);
    int Offset = 0;
    for (int Idx = 0; Idx < RecordingBlob->NumChunks; Idx++){
        char ChunkKey[50];
        sprintf(ChunkKey, "%s_%d", RecordingBlob->UUID+25, Idx);
        if (NVSReadAndErase(PayloadSpeex + Offset, SpeexOutFrameSize, ChunkKey) == true){
            Offset += SpeexOutFrameSize;
        } else {
            continue;
        };
    };
    if (Offset == 0){
        ESP_LOGE(TAG,"Empty message, nothing to propogate..");
        return;
    };
    int PayloadBase64Size = B64ENCODE_OUT_SAFESIZE(PayloadSpeexSize);
    size_t Outlen;
    const char *PayloadBase64 = malloc(PayloadBase64Size);
    switch (mbedtls_base64_encode(PayloadBase64, PayloadBase64Size, &Outlen, PayloadSpeex, PayloadSpeexSize)){
        case MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL:
            ESP_LOGE(TAG,"MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL");
            return;
        case MBEDTLS_ERR_BASE64_INVALID_CHARACTER:
            ESP_LOGE(TAG,"MBEDTLS_ERR_BASE64_INVALID_CHARACTER");
            return;
        default:
            break;
    };
    cJSON_AddStringToObject(PropogateObject, "Payload", PayloadBase64);
    char *JsonStringToSend = cJSON_PrintUnformatted(PropogateObject);
    if (esp_websocket_client_is_connected(WebSocketClient)) {
        esp_websocket_client_send_text(WebSocketClient, JsonStringToSend, strlen(JsonStringToSend), portMAX_DELAY);
    };
    free(PayloadSpeex);
    free(PayloadBase64);
    free(JsonStringToSend);
    cJSON_Delete(PropogateObject);
};

char StartMark[] = "F3SB";
char StopMark[]  = "F3FN";

void Fau3PropogateStreamPacket(char *buffer, int len){
    char *OutgoingBuffer = malloc(len+10);
    int PacketLength = 8 + len;
    memcpy(OutgoingBuffer,StartMark,4);
    memcpy(OutgoingBuffer+4,buffer,len);
    memcpy(OutgoingBuffer+4+len,StopMark,4);
    if (esp_websocket_client_is_connected(WebSocketClient)) {
        esp_websocket_client_send_text(WebSocketClient, OutgoingBuffer, PacketLength, portMAX_DELAY);
    };
    free(OutgoingBuffer);
};

void Fau3Ping(){
    cJSON *PingObject = cJSON_CreateObject();
    cJSON_AddStringToObject(PingObject, "MessageType", "Ping");
    char PingUUID[37]; GenerateUUIDv4(PingUUID);
    cJSON_AddStringToObject(PingObject, "MessageUuid", PingUUID);
    char *JsonStringToSend = cJSON_PrintUnformatted(PingObject);
    if (esp_websocket_client_is_connected(WebSocketClient)) {
        esp_websocket_client_send_text(WebSocketClient, JsonStringToSend, strlen(JsonStringToSend), portMAX_DELAY);
    };
    free(JsonStringToSend);
    cJSON_Delete(PingObject);
};

TaskHandle_t Fau3ChangeSubscriptionTask;
bool IsSubscriptionRunning = false;

void Fau3Subscribe(void* Arg){
    unsigned short _SelectedLine = SelectedLine;
    for (short Idx = 0; Idx < 10; Idx++){
        if (_SelectedLine != SelectedLine){
            _SelectedLine = SelectedLine;
            Idx = 0;
            continue;
        };
        vTaskDelay(pdMS_TO_TICKS(100));
    };
    ESP_LOGI(TAG, "Selected line is ready to be confirmed: %d", SelectedLine);
    cJSON *SubscribeObject = cJSON_CreateObject();
    cJSON_AddStringToObject(SubscribeObject, "MessageType", "Subscribe");
    char SubscribeUUID[37]; GenerateUUIDv4(SubscribeUUID);
    cJSON_AddStringToObject(SubscribeObject, "MessageUuid", SubscribeUUID);
    char RoomId[20]; sprintf(RoomId,"Channel%d",SelectedLine);
    cJSON_AddStringToObject(SubscribeObject, "RoomId", RoomId);
    cJSON_AddNumberToObject(SubscribeObject, "IsPingRequired", CONFIG_FAU3_PING_INTERVAL);
    char *JsonStringToSend = cJSON_PrintUnformatted(SubscribeObject);
    if (esp_websocket_client_is_connected(WebSocketClient)) {
        esp_websocket_client_send_text(WebSocketClient, JsonStringToSend, strlen(JsonStringToSend), portMAX_DELAY);
    };
    free(JsonStringToSend);
    cJSON_Delete(SubscribeObject);
    IsSubscriptionRunning = false;
    vTaskDelete(NULL);
};

void Fau3Confirm(char *UUID, int Status){
    cJSON *SubscribeObject = cJSON_CreateObject();
    cJSON_AddStringToObject(SubscribeObject, "MessageType", "Confirm");
    cJSON_AddStringToObject(SubscribeObject, "MessageUuid", UUID);
    cJSON_AddNumberToObject(SubscribeObject, "Status", Status);
    char *JsonStringToSend = cJSON_PrintUnformatted(SubscribeObject);
    if (esp_websocket_client_is_connected(WebSocketClient)) {
        esp_websocket_client_send_text(WebSocketClient, JsonStringToSend, strlen(JsonStringToSend), portMAX_DELAY);
    };
    free(JsonStringToSend);
    cJSON_Delete(SubscribeObject);
};

TaskHandle_t Fau3RequestsHandler;

void Fau3RequestHandler(void* Arg) {
    char *Data = (char *)Arg;
    cJSON* IncomingJsonObject = cJSON_Parse(Data);
    if (IncomingJsonObject == NULL){
        if (strstr(Data,StartMark) != NULL && strstr(Data,StopMark) != NULL){
            AppendStreamChunk((unsigned char *)&Data[4]);
        }else{
            ESP_LOGE(TAG,"Invalid packet received");
        };
    }else{
        cJSON* MessageTypeObject = cJSON_GetObjectItem(IncomingJsonObject, "MessageType");
        cJSON* MessageUuidObject = cJSON_GetObjectItem(IncomingJsonObject, "MessageUuid");
        if (IsUuidCached(MessageUuidObject->valuestring) == false) {
            AddUuidToCache(MessageUuidObject->valuestring);
            if (strcmp(MessageTypeObject->valuestring, "Playlist") == 0) {
                cJSON* RecordsObject = cJSON_GetObjectItem(IncomingJsonObject, "Records");
                if (cJSON_IsArray(RecordsObject)) {
                    int RecordsCount = cJSON_GetArraySize(RecordsObject);
                    for (short Idx = 0; Idx < RecordsCount; Idx++) {
                        cJSON* SingleRecordMetaJsonObject = cJSON_GetArrayItem(RecordsObject, Idx);
                        // cJSON* MessageClientObject = cJSON_GetObjectItem(SingleRecordMetaJsonObject, "ClientId");
                        cJSON* MessagePayloadObject = cJSON_GetObjectItem(SingleRecordMetaJsonObject, "Payload");
                        int PayloadBase64Size = strlen(MessagePayloadObject->valuestring);
                        int PayloadSpeexSizeSave = B64DECODE_OUT_SAFESIZE(PayloadBase64Size);
                        unsigned char *PayloadSpeex = malloc(PayloadSpeexSizeSave);
                        size_t Outlen;
                        mbedtls_base64_decode(PayloadSpeex, PayloadSpeexSizeSave, &Outlen,
                                                (const unsigned char *)MessagePayloadObject->valuestring, PayloadBase64Size);
                        if (Outlen > 0){
                            AppendPlaylistRecord(PayloadSpeex, Outlen);
                        };
                        free(PayloadSpeex);
                    };
                    Fau3Confirm(MessageUuidObject->valuestring, FAU3_STORED);
                };
            } else if (strcmp(MessageTypeObject->valuestring, "Ping") == 0) {
                Fau3Confirm(MessageUuidObject->valuestring, FAU3_PONG);
            };
        };
        cJSON_Delete(IncomingJsonObject);
    };
    RemoveExpiredUuidEntries();
    vTaskDelete(NULL);
};

char *WebSocketMessageBuffer = NULL;

static void WebsocketEventHandler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data){
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
    switch (event_id) {
    case WEBSOCKET_EVENT_CONNECTED:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_CONNECTED");
        IsSubscriptionRunning = true;
        xTaskCreate(Fau3Subscribe, "Fau3Subscribe", 4096, NULL, 1, &Fau3ChangeSubscriptionTask);
        // xTaskCreate(SetStatusLedConnectd, "SetStatusLedConnectd", 4096, NULL, 1, &TangentaStatusLedHandle);
        break;
    case WEBSOCKET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_DISCONNECTED");
        break;
    case WEBSOCKET_EVENT_DATA:
        // ESP_LOGI(TAG, "WEBSOCKET_EVENT_DATA");
        int CurrentBufferLength = data->data_len;
        int TotalBufferLength = data->payload_len;
        int CurrentOffset = data->payload_offset;
        if (data->payload_len > 0){
            if (CurrentOffset == 0){
                if (TotalBufferLength == CurrentBufferLength){
                    xTaskCreate(Fau3RequestHandler, "Fau3RequestHandler", 4096, (void *)data->data_ptr, 1, &Fau3RequestsHandler);
                } else if (TotalBufferLength > CurrentBufferLength){
                    WebSocketMessageBuffer = malloc(TotalBufferLength);
                    memcpy(WebSocketMessageBuffer, data->data_ptr, CurrentBufferLength);
                };
            } else {
                memcpy(WebSocketMessageBuffer + CurrentOffset, data->data_ptr, CurrentBufferLength);
                if ( CurrentOffset + CurrentBufferLength == TotalBufferLength ){
                    xTaskCreate(Fau3RequestHandler, "Fau3RequestHandler", 4096, (void *)WebSocketMessageBuffer, 1, &Fau3RequestsHandler);
                    vTaskDelay(pdMS_TO_TICKS(100));
                    if (WebSocketMessageBuffer != NULL){
                        free(WebSocketMessageBuffer);
                    };
                };
            };
        };
        break;
    case WEBSOCKET_EVENT_ERROR:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_ERROR");
        // xTaskCreate(SetStatusLedNoSignal, "SetStatusLedNoSignal", 4096, NULL, 1, &TangentaStatusLedHandle);
        break;
    };
};

void ConfigureWebSocket(){
    websocket_cfg.uri                   = CONFIG_WEBSOCKET_URI;
    websocket_cfg.reconnect_timeout_ms  = 1000;
    websocket_cfg.network_timeout_ms    = 1000;
    websocket_cfg.buffer_size           = B64ENCODE_OUT_SAFESIZE(MaximumRecordBlobSize * SpeexOutFrameSize);
    char ClientHeader[50];
    sprintf(ClientHeader, "fau3-client-id: %s\r\n", UniqueIdSrt);
    websocket_cfg.headers = ClientHeader;
    WebSocketClient = esp_websocket_client_init(&websocket_cfg);
    esp_websocket_register_events(WebSocketClient, WEBSOCKET_EVENT_ANY, WebsocketEventHandler, (void *)WebSocketClient);
    esp_websocket_client_start(WebSocketClient);
};

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

audio_pipeline_handle_t RecPipeline, PlayPipeline;

SpeexBits speex_encoder_bits, speex_decoder_bits;
void *speex_encoder_state;
void *speex_decoder_state;

static esp_err_t SpeexEncoderOpen(audio_element_handle_t self){
    speex_bits_init(&speex_encoder_bits);
    speex_encoder_state = speex_encoder_init(&speex_nb_mode);
    int _AudioSampleRate = AudioSampleRate;
    speex_encoder_ctl(speex_encoder_state ,SPEEX_SET_SAMPLING_RATE,&_AudioSampleRate);
    int _SpeexQualityLevel = SpeexQualityLevel;
    speex_encoder_ctl(speex_encoder_state, SPEEX_SET_QUALITY, &_SpeexQualityLevel);
    GenerateUUIDv4(RecordingBlob.UUID);
    ESP_LOGI(TAG,"Recording new message with UUID=%s",RecordingBlob.UUID);
    RecordingBlob.NumChunks  = 0;
    RecordingBlob.TotalSize = 0;
    return ESP_OK;
};
static esp_err_t SpeexEncoderClose(audio_element_handle_t self){
    speex_bits_destroy(&speex_encoder_bits);
    speex_encoder_destroy(speex_encoder_state);
    if (IsStreamModeActivated == false){
        ESP_LOGI(TAG,"Recording blob is ready to be sent");
        Fau3Propogate(&RecordingBlob);
    };
    return ESP_OK;
};
static int SpeexEncoderProcess(audio_element_handle_t self, char *in_buffer, int in_len){
    int out_len = 0;
    int rsize = audio_element_input(self, in_buffer, in_len);
    if (rsize > 0 && in_len <= SpeexInFrameSize){
            signed short FrameToEncode[SpeexInFrameSize/sizeof(short)];
            memcpy(FrameToEncode,in_buffer, SpeexInFrameSize);
            speex_bits_reset(&speex_encoder_bits);
            speex_encode_int(speex_encoder_state, FrameToEncode, &speex_encoder_bits);
            out_len = speex_bits_nbytes(&speex_encoder_bits);
            char out_buffer[out_len];
            speex_bits_write(&speex_encoder_bits, out_buffer, out_len);
            int wsize = audio_element_output(self, out_buffer, out_len);
            // ESP_LOGI(TAG,"[OK]: Frame encoded %d -> %d",rsize,wsize);
            return wsize;
    };
    return 0;
};

static int StoreEncodedPacket(audio_element_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context){
    char ChunkKey[50];
    sprintf(ChunkKey, "%s_%d", RecordingBlob.UUID+25, RecordingBlob.NumChunks);
    if (IsStreamModeActivated == true){
        Fau3PropogateStreamPacket(buffer,len);
    }else{
        if (NVSWrite((unsigned char *)buffer, len, ChunkKey) == true){
            RecordingBlob.NumChunks++;
            RecordingBlob.TotalSize += len;
        } else {
            return 0;
        };
    };
    return len;
};

static esp_err_t SpeexDecoderOpen(audio_element_handle_t self){
    speex_bits_init(&speex_decoder_bits);
    speex_decoder_state = speex_decoder_init(&speex_nb_mode);
    int Enhancement = 1;
    speex_decoder_ctl(speex_decoder_state, SPEEX_SET_ENH, &Enhancement);
    return ESP_OK;
};
static esp_err_t SpeexDecoderClose(audio_element_handle_t self){
    speex_bits_destroy(&speex_decoder_bits);
    speex_decoder_destroy(speex_decoder_state);
    return ESP_OK;
};
static int  SpeexDecoderProcess(audio_element_handle_t self, char *in_buffer, int in_len){
    char out_buffer[SpeexInFrameSize];
    int rsize = audio_element_input(self, in_buffer, in_len);
    if (rsize > 0  && in_len <= SpeexOutFrameSize){
        speex_bits_read_from(&speex_decoder_bits, (char *)in_buffer, in_len);
        signed short  DecodedFrame[SpeexInFrameSize/sizeof(short)];
        switch (speex_decode_int(speex_decoder_state, &speex_decoder_bits, (spx_int16_t *)DecodedFrame)){
            case -1:
                ESP_LOGE(TAG,"[SPEEX]: END_OF_STREAM");
                return 0;
            case -2:
                ESP_LOGE(TAG,"[SPEEX]: CORRUPT_STREAM");
                return 0;
        };
        memcpy(out_buffer,DecodedFrame,SpeexInFrameSize);
        int wsize = audio_element_output(self, out_buffer, SpeexInFrameSize);
        // ESP_LOGI(TAG,"[OK]: Frame decoded %d -> %d",rsize, wsize);
        return wsize;
    };
    return 0;
};
static int ExtractEncodedPacket(audio_element_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context){
    len = 0;

    if (Stream.IsReceivingStream == false){
        char ChunkKey[50];
        if (Playlist.PlayingRecord->ReadIndex >= Playlist.PlayingRecord->NumChunks){
            ESP_LOGI(TAG,"Playing completed, waiting for next record..");
            return 0;
        };
        sprintf(ChunkKey, "%s_%d", Playlist.PlayingRecord->UUID+25, Playlist.PlayingRecord->ReadIndex++);
        printf("[OK]: Reading new chunk from NVS:%s\n", ChunkKey);
        if (NVSReadAndErase((unsigned char *)buffer, SpeexOutFrameSize, ChunkKey) != true){
            return ExtractEncodedPacket(self, buffer, len, ticks_to_wait, context);
        };
        return SpeexOutFrameSize;
    } else {
        len = 0;
        if(xSemaphoreTake(Stream.ReadWriteSema, portMAX_DELAY)){
            ESP_LOGI(TAG,"[STREAM-READ]: BufferSize = %d", Stream.BufferSize);
            if (Stream.BufferSize >= SpeexOutFrameSize){
                memcpy(buffer, Stream.StreamData, SpeexOutFrameSize);
                Stream.BufferSize -= SpeexOutFrameSize;
                memmove(Stream.StreamData, Stream.StreamData+SpeexOutFrameSize, Stream.BufferSize);
                Stream.StreamData = realloc(Stream.StreamData, Stream.BufferSize);
                len = SpeexOutFrameSize;
            };
            xSemaphoreGive(Stream.ReadWriteSema);
        };
    };
    return len;
};

audio_element_handle_t I2S_StreamWriter, I2S_StreamReader;

void ConfigureAudioPipelines(){
    audio_board_handle_t board_handle = audio_board_init();
    audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_BOTH, AUDIO_HAL_CTRL_START);
    audio_hal_set_volume(board_handle->audio_hal, 100);
    //-------------------------------------------------------
    i2s_stream_cfg_t i2s_cfg_rec = I2S_STREAM_CFG_DEFAULT();
    i2s_cfg_rec.i2s_config.sample_rate = AudioSampleRate;
    i2s_cfg_rec.i2s_config.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    i2s_cfg_rec.i2s_config.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
    i2s_cfg_rec.type = AUDIO_STREAM_READER;
    i2s_cfg_rec.out_rb_size = 16 * 1024; // Increase buffer to avoid missing data in bad network conditions
    I2S_StreamReader = i2s_stream_init(&i2s_cfg_rec);

    audio_element_cfg_t encoder_cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    encoder_cfg.task_stack = 16384;
    encoder_cfg.buffer_len = SpeexInFrameSize;
    encoder_cfg.open = SpeexEncoderOpen;
    encoder_cfg.close = SpeexEncoderClose;
    encoder_cfg.process = SpeexEncoderProcess;
    encoder_cfg.write = StoreEncodedPacket;
    audio_element_handle_t SpeexEncStream = audio_element_init(&encoder_cfg);

    audio_pipeline_cfg_t rec_pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    RecPipeline = audio_pipeline_init(&rec_pipeline_cfg);
    mem_assert(RecPipeline);
    audio_pipeline_register(RecPipeline, I2S_StreamReader, "Record");
    audio_pipeline_register(RecPipeline, SpeexEncStream,   "Encode");
    const char *rec_link_tag[2] = {"Record", "Encode"};
    audio_pipeline_link(RecPipeline, &rec_link_tag[0],  2);
    //-------------------------------------------------------
    i2s_stream_cfg_t i2s_cfg_play = I2S_STREAM_CFG_DEFAULT();
    i2s_cfg_play.i2s_config.sample_rate = AudioSampleRate;
    i2s_cfg_play.i2s_config.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    i2s_cfg_play.i2s_config.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
    i2s_cfg_play.type = AUDIO_STREAM_WRITER;
    i2s_cfg_play.task_core = 1;
    i2s_cfg_play.task_prio = configMAX_PRIORITIES -1;
    I2S_StreamWriter = i2s_stream_init(&i2s_cfg_play);

    audio_element_cfg_t decoder_cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    decoder_cfg.task_stack = 8192;
    decoder_cfg.buffer_len = SpeexOutFrameSize;
    decoder_cfg.open = SpeexDecoderOpen;
    decoder_cfg.close = SpeexDecoderClose;
    decoder_cfg.process = SpeexDecoderProcess;
    decoder_cfg.read = ExtractEncodedPacket;
    audio_element_handle_t SpeexDecStream = audio_element_init(&decoder_cfg);

    audio_pipeline_cfg_t play_pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    PlayPipeline = audio_pipeline_init(&play_pipeline_cfg);
    mem_assert(PlayPipeline);
    audio_pipeline_register(PlayPipeline, SpeexDecStream,   "Decode");
    audio_pipeline_register(PlayPipeline, I2S_StreamWriter, "Playback");
    const char *play_link_tag[2] = {"Decode", "Playback"};
    audio_pipeline_link(PlayPipeline, &play_link_tag[0],  2);
};

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

#include "driver/gpio.h"

#define RotaryEncoderAP  19
#define RotaryEncoderAN  22
#define RotaryEncoderBP  21
#define RotaryEncoderBN  27


#define TangentaOne 2
#define TangentaTwo 4

const static int logic_ap[] = {1,0,0,1,1,0,0,1,1,1,0,0,1,1,0,0};
const static int logic_an[] = {0,0,1,0,1,1,0,0,1,1,0,0,1,0,1,1};
const static int logic_bp[] = {0,0,1,1,0,0,1,1,1,0,0,1,1,0,0,1};
const static int logic_bn[] = {1,1,0,0,0,0,1,1,1,1,0,0,0,0,1,1};


void GetEncoderState(){
    short ap = gpio_get_level(RotaryEncoderAP);
    short an = gpio_get_level(RotaryEncoderAN);
    short bp = gpio_get_level(RotaryEncoderBP);
    short bn = gpio_get_level(RotaryEncoderBN);
    for (SelectedLine=0;SelectedLine<16;SelectedLine++){
        if (ap == logic_ap[SelectedLine] &&
            an == logic_an[SelectedLine] &&
            bp == logic_bp[SelectedLine] &&
            bn == logic_bn[SelectedLine])
            break;
    };
    SelectedLine++;
};


static void IRAM_ATTR RotaryEncoderISRHandler(void* arg){
    GetEncoderState();
    if (IsSubscriptionRunning == false){
        IsSubscriptionRunning  = true;
        xTaskCreate(Fau3Subscribe, "Fau3Subscribe", 4096, NULL, 1, &Fau3ChangeSubscriptionTask);
    };
    ets_printf("Line value has been changed:%d\n",SelectedLine);
};

TaskHandle_t RecTaskHandle, PlayTaskHandle;
static bool IsTangentaOnePressed = false;
static bool IsTangentaTwoPressed = false;

// void RunRecordPipeline(void* arg) {
//     while (PlayPipeline.WorkingStatus == true){
//         vTaskDelay(pdMS_TO_TICKS(20));
//     };
//     vTaskDelay(pdMS_TO_TICKS(500));
//     if (RecPipeline.WorkingStatus == false && (IsTangentaOnePressed == true || IsTangentaTwoPressed == true)){
//         audio_pipeline_run(RecPipeline.Pipeline);
//         RecPipeline.WorkingStatus = true;
//     };
//     vTaskDelete(NULL);
// };
// void StopRecordPipeline(void* arg){
//     if (RecPipeline.WorkingStatus == true){
//         vTaskDelay(pdMS_TO_TICKS(500));
//         audio_pipeline_stop(RecPipeline.Pipeline);
//         audio_pipeline_wait_for_stop(RecPipeline.Pipeline);
//         audio_pipeline_terminate(RecPipeline.Pipeline);
//         audio_pipeline_reset_ringbuffer(RecPipeline.Pipeline);
//         audio_pipeline_reset_elements(RecPipeline.Pipeline);
//         RecPipeline.WorkingStatus = false;
//     };
//     vTaskDelete(NULL);
// };


void RunRecorder(void* Arg){
    ESP_LOGI(TAG, "[RECORDER]: Recorder started");
    while (1){
        audio_element_state_t ElementState = audio_element_get_state(I2S_StreamReader);
        switch (ElementState) {
            case AEL_STATE_INIT :
                // ESP_LOGI(TAG,"[RECORDER]: AEL_STATE_INIT");
                if (IsTangentaOnePressed == true || IsTangentaTwoPressed == true){
                    if (audio_pipeline_get_state(PlayPipeline) == AEL_STATE_RUNNING){
                        audio_pipeline_stop(PlayPipeline);
                        audio_pipeline_wait_for_stop(PlayPipeline);
                        audio_pipeline_terminate(PlayPipeline);
                        audio_pipeline_reset_ringbuffer(PlayPipeline);
                        audio_pipeline_reset_elements(PlayPipeline);
                        audio_pipeline_change_state(PlayPipeline, AEL_STATE_INIT);
                    } else {
                        audio_pipeline_run(RecPipeline);
                    };
                };
                break;
            case AEL_STATE_RUNNING :
                ESP_LOGI(TAG,"[RECORDER]: AEL_STATE_RUNNING");
                if (IsTangentaOnePressed != true && IsTangentaTwoPressed != true){
                    audio_pipeline_stop(RecPipeline);
                };
                break;
            case AEL_STATE_STOPPED :
                ESP_LOGI(TAG,"[RECORDER]: AEL_STATE_STOPPED");
                audio_pipeline_terminate(RecPipeline);
                audio_pipeline_reset_ringbuffer(RecPipeline);
                audio_pipeline_reset_elements(RecPipeline);
                audio_pipeline_change_state(RecPipeline, AEL_STATE_INIT);
                break;
            default :
                break;
        };
        vTaskDelay(pdMS_TO_TICKS(100));
    };
};

void RunPlayer(void* Arg) {
    ESP_LOGI(TAG, "[PLAYER]: Player started");
    while (1){
        audio_element_state_t ElementState = audio_element_get_state(I2S_StreamWriter);
        switch (ElementState) {
            case AEL_STATE_INIT :
                // ESP_LOGI(TAG,"[PLAYER]: AEL_STATE_INIT");
                if (audio_pipeline_get_state(RecPipeline) == AEL_STATE_INIT){
                    if (Stream.IsReceivingStream == false){
                        if (xSemaphoreTake(Playlist.PlaylistUpdateSemaphore, pdMS_TO_TICKS(500))){
                            if (Playlist.PlayingRecord != NULL ){
                                memmove(&Playlist.Records[0], &Playlist.Records[1],
                                        sizeof(SingleRecordMeta) * (Playlist.CountRecords - 1));
                                Playlist.CountRecords--;
                                Playlist.PlayingRecord = NULL;
                            };
                            if (Playlist.CountRecords > 0 ){
                                ESP_LOGI(TAG,"[PLAYER]: Playlist is not empty, so starting player..");
                                Playlist.PlayingRecord = &Playlist.Records[0];
                                audio_pipeline_run(PlayPipeline);
                            };
                            xSemaphoreGive(Playlist.PlaylistUpdateSemaphore);
                            break;
                        };
                    } else {
                        struct timeval CurrentTime;
                        gettimeofday(&CurrentTime, NULL);
                        uint64_t CurrentTimeMSec = CurrentTime.tv_sec * 1000000ULL + CurrentTime.tv_usec;
                        if (xSemaphoreTake(Stream.ReadWriteSema, pdMS_TO_TICKS(500))){
                            uint64_t LastReceivedTime = Stream.LastReceivedTime.tv_sec * 1000000ULL + Stream.LastReceivedTime.tv_usec;
                            if ((Stream.BufferSize > 0 && (CurrentTimeMSec - LastReceivedTime > 500000ULL)) ||
                                (Stream.BufferSize >= 50 * SpeexOutFrameSize)){
                                audio_pipeline_run(PlayPipeline);
                            } else if (CurrentTimeMSec - LastReceivedTime > 2000000ULL && Stream.BufferSize == 0){
                                ESP_LOGI(TAG,"[PLAYER]: Stream timeout reached..");
                                Stream.IsReceivingStream = false;
                                if (Stream.StreamData != NULL){
                                    free(Stream.StreamData);
                                    Stream.BufferSize = 0;
                                };
                            };
                            xSemaphoreGive(Stream.ReadWriteSema);
                        };
                    };
                };
                break;
            case AEL_STATE_RUNNING :
                ESP_LOGI(TAG,"[PLAYER]: AEL_STATE_RUNNING");
                break;
            case AEL_STATE_FINISHED :
                ESP_LOGI(TAG,"[PLAYER]: AEL_STATE_FINISHED");
                audio_pipeline_terminate(PlayPipeline);
                audio_pipeline_reset_ringbuffer(PlayPipeline);
                audio_pipeline_reset_elements(PlayPipeline);
                audio_pipeline_change_state(PlayPipeline, AEL_STATE_INIT);
                break;
            case AEL_STATE_ERROR:
                ESP_LOGI(TAG,"[PLAYER]: AEL_STATE_ERROR");
                break;
            default :
                break;
        };
        vTaskDelay(pdMS_TO_TICKS(100));
        // size_t FreeHeapSize = esp_get_free_heap_size();
        // ESP_LOGI(TAG, "Free RAM Size: %6d", FreeHeapSize);
    };
};



static void IRAM_ATTR TangentaInterruptHandler(void* arg) {
    int TOneLevel = gpio_get_level(TangentaOne);
    int TTwoLevel = gpio_get_level(TangentaTwo);
    if (TOneLevel == 1){
        if (IsTangentaOnePressed == true){
            // xTaskCreate(StopRecordPipeline, "StopRecordPipeline", 4096, NULL, 1, &RecTaskHandle);
            ets_printf("TangentaOne RELEASED\n");
            IsTangentaOnePressed = false;
        };
    };
    if (TTwoLevel == 1){
        if (IsTangentaTwoPressed == true){
            // xTaskCreate(StopRecordPipeline, "StopRecordPipeline", 4096, NULL, 1, &RecTaskHandle);
            ets_printf("TangentaTwo RELEASED\n");
            IsTangentaTwoPressed = false;
        };
    };
    if (TOneLevel == 0 && TTwoLevel == 1){
        if (IsTangentaOnePressed == false){
            IsStreamModeActivated = false;
            // xTaskCreate(RunRecordPipeline, "RunRecordPipeline", 4096, NULL, 1, &RecTaskHandle);
            ets_printf("TangentaOne PRESSED\n");
            IsTangentaOnePressed = true;
        };
    } else if (TOneLevel == 1 && TTwoLevel == 0){
        if (IsTangentaTwoPressed == false){
            IsStreamModeActivated = true;
            // xTaskCreate(RunRecordPipeline, "RunRecordPipeline", 4096, NULL, 1, &RecTaskHandle);
            ets_printf("TangentaTwo PRESSED\n");
            IsTangentaTwoPressed = true;
        };
        return;
    };
};

void ConfigureDiginalInputs(){
    // gpio_install_isr_service(0);
    gpio_set_pull_mode(RotaryEncoderAP, GPIO_PULLUP_ONLY);
    gpio_set_direction(RotaryEncoderAP, GPIO_MODE_INPUT);
    gpio_set_intr_type(RotaryEncoderAP, GPIO_INTR_ANYEDGE);
    gpio_isr_handler_add(RotaryEncoderAP, RotaryEncoderISRHandler, NULL);
    //-------------------------------------------------------------------
    gpio_set_pull_mode(RotaryEncoderAN, GPIO_PULLUP_ONLY);
    gpio_set_direction(RotaryEncoderAN, GPIO_MODE_INPUT);
    gpio_set_intr_type(RotaryEncoderAN, GPIO_INTR_ANYEDGE);
    gpio_isr_handler_add(RotaryEncoderAN, RotaryEncoderISRHandler, NULL);
    //-------------------------------------------------------------------
    gpio_set_pull_mode(RotaryEncoderBP, GPIO_PULLUP_ONLY);
    gpio_set_direction(RotaryEncoderBP, GPIO_MODE_INPUT);
    gpio_set_intr_type(RotaryEncoderBP, GPIO_INTR_ANYEDGE);
    gpio_isr_handler_add(RotaryEncoderBP, RotaryEncoderISRHandler, NULL);
    //-------------------------------------------------------------------
    gpio_set_pull_mode(RotaryEncoderBN, GPIO_PULLUP_ONLY);
    gpio_set_direction(RotaryEncoderBN, GPIO_MODE_INPUT);
    gpio_set_intr_type(RotaryEncoderBN, GPIO_INTR_ANYEDGE);
    gpio_isr_handler_add(RotaryEncoderBN, RotaryEncoderISRHandler, NULL);
    //-------------------------------------------------------------------
    // gpio_set_direction(TangentaStatusLed, GPIO_MODE_OUTPUT);
    //===================================================================
    gpio_config_t TangentaOneConfig;
    TangentaOneConfig.intr_type = GPIO_INTR_ANYEDGE;
    TangentaOneConfig.mode = GPIO_MODE_INPUT;
    TangentaOneConfig.pin_bit_mask = (1ULL << TangentaOne);
    gpio_config(&TangentaOneConfig);
    gpio_isr_handler_add(TangentaOne, TangentaInterruptHandler, NULL);
    //===================================================================
    gpio_config_t TangentaTwoConfig;
    TangentaTwoConfig.intr_type = GPIO_INTR_ANYEDGE;
    TangentaTwoConfig.mode = GPIO_MODE_INPUT;
    TangentaTwoConfig.pin_bit_mask = (1ULL << TangentaTwo);
    gpio_config(&TangentaTwoConfig);
    gpio_isr_handler_add(TangentaTwo, TangentaInterruptHandler, NULL);
    GetEncoderState();
    //-------------------------------------------------------------------
};

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

void app_main(void){
    nvs_flash_erase();
    ESP_ERROR_CHECK( nvs_flash_init() );
    nvs_open(TAG, NVS_READWRITE, &NVS_Handle);
    nvs_erase_all(NVS_Handle);
    Playlist.PlaylistUpdateSemaphore = xSemaphoreCreateMutex();
    NVSMutex                         = xSemaphoreCreateMutex();
    Stream.ReadWriteSema             = xSemaphoreCreateMutex();
    SetUniqueIdSrt();
    ConfigureWiFi();
    ConfigureAudioPipelines();
    ConfigureDiginalInputs();
    ConfigureWebSocket();

    xTaskCreate(RunPlayer,   "RunPlayer",   8192, NULL, 1, &PlayTaskHandle);
    xTaskCreate(RunRecorder, "RunRecorder", 8192, NULL, 1, &RecTaskHandle );
    // while (1){
    //     vTaskDelay(pdMS_TO_TICKS(1000));
    // };
    /* Release all resources */
    // nvs_close(NVS_Handle);
    // audio_pipeline_deinit(pipeline);
    // audio_element_deinit(I2S_StreamReader);
    // audio_element_deinit(I2S_StreamWriter);
};
