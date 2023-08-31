
#include    "fau3vomlib.h"
#include  <sys/resource.h>

Fau3ActiveClients ActiveClients = {
    .TotalCount = 0
};

static int Fau3Callback( struct lws *WebsocketInstance, enum lws_callback_reasons Event, void *User, void *_IncomingData, size_t Length ){
    switch( Event ){
        case LWS_CALLBACK_HTTP_CONFIRM_UPGRADE:
            printf("[OK]: LWS_CALLBACK_HTTP_CONFIRM_UPGRADE \n");
            char ClientHeader[] = "fau3-client-id:";
            int IsHeaderFound = lws_hdr_custom_length(WebsocketInstance, ClientHeader, sizeof(ClientHeader)-1);
            if (IsHeaderFound < 0){
                lwsl_notice("%s: Can't find %s\n", __func__, ClientHeader);
                lws_callback_on_writable(WebsocketInstance);
                return -1;
            } else {
                char ClientUniqueId[18];
                if (lws_hdr_custom_copy(WebsocketInstance, ClientUniqueId, sizeof(ClientUniqueId), ClientHeader, sizeof(ClientHeader)-1) > 0){
                    printf("[OK][AUTH]: Fau3 client header found:%s. New connection accepted\n", ClientUniqueId);
                    Fau3AppendClient(&ActiveClients, WebsocketInstance, ClientUniqueId);
                };
            };
            break;
        case LWS_CALLBACK_HTTP_WRITEABLE:
            const char *ErrorResponse = "HTTP/1.1 400 Bad Request\r\n\r\n";
            lws_write(WebsocketInstance, (unsigned char *)ErrorResponse, strlen(ErrorResponse), LWS_WRITE_HTTP);
            lws_close_reason(WebsocketInstance, LWS_CLOSE_STATUS_PROTOCOL_ERR, NULL, 0);
            return -1;
        case LWS_CALLBACK_ESTABLISHED:
            printf("[OK]: LWS_CALLBACK_ESTABLISHED \n");
            lws_set_timer_usecs(WebsocketInstance, LWS_USEC_PER_SEC);
            break;
        case LWS_CALLBACK_RECEIVE:
            unsigned char *IncomingData = (unsigned char*)malloc(LWS_SEND_BUFFER_PRE_PADDING + Length + LWS_SEND_BUFFER_POST_PADDING);
            memcpy( &IncomingData[LWS_SEND_BUFFER_PRE_PADDING], _IncomingData, Length );
            printf("LWS_CALLBACK_RECEIVE: %4d (rpp %5d, last %d)\n",
                   (int)Length, (int)lws_remaining_packet_payload(WebsocketInstance),
                   lws_is_final_fragment(WebsocketInstance));
            Fau3ClientIncomingBufferAppend(&ActiveClients, WebsocketInstance, &IncomingData[LWS_SEND_BUFFER_PRE_PADDING], Length);
            free(IncomingData);
            break;
        case LWS_CALLBACK_TIMER:
            lws_set_timer_usecs(WebsocketInstance, LWS_USEC_PER_SEC);
            Fau3SetSubscriptionRequestsFlags(&ActiveClients, WebsocketInstance);
            break;
        case LWS_CALLBACK_SERVER_WRITEABLE:
            Fau3GetSubscriptionRequestsFlags(&ActiveClients, WebsocketInstance);
            Fau3SendCollectedPlaylist(&ActiveClients, WebsocketInstance);
            break;
        case LWS_CALLBACK_CLOSED:
            printf("[OK]: LWS_CALLBACK_CLOSED \n");
            Fau3DeleteClient(&ActiveClients, WebsocketInstance);
            break;
        default:
            break;
    }
    return 0;
}

enum Protocols{
    PROTOCOL_FAU3 = 0
};

static struct lws_protocols Protocols[] = {
    { "Fau3VOMProto", Fau3Callback, 0 },
    { NULL, NULL, 0, 0 } /* terminator */
};

int main( int argc, char *argv[] ){

    struct rlimit limit;
    int result = getrlimit(RLIMIT_DATA, &limit);
    if (result == 0) {
        printf("Maximum heap size: %lu bytes\n", limit.rlim_cur);
    } else {
        printf("Failed to retrieve the maximum heap size.\n");
    };

    ActiveClients.TotalCount = 0;
    struct lws_context_creation_info LwsInfo;
    memset( &LwsInfo, 0, sizeof(LwsInfo) );
    LwsInfo.port = FAU3_CLIENT_ROUTER_WS_PORT;
    LwsInfo.protocols = Protocols;
    LwsInfo.gid = -1;
    LwsInfo.uid = -1;
    pthread_mutex_init(&ActiveClients.ClientsLock, NULL);
    struct lws_context *context = lws_create_context( &LwsInfo );
    while( 1 ){
        lws_service( context, /* timeout_ms = */ 1000 );
    };
    lws_context_destroy( context );
    return 0;
}
