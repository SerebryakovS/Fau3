
// Usage: ./fau3vom


#include    "fau3vomlib.h"
#include <libwebsockets.h>

unsigned char *OutgoingData;

static int Fau3Callback( struct lws *WebsocketInstance, enum lws_callback_reasons Event, void *User, void *_IncomingData, size_t Length ){
    switch( Event ){
        case LWS_CALLBACK_RECEIVE: ;
            char IncomingData[FAU3_BUFFER_BYTES];
            if (strlen((char *)_IncomingData) > 0){
                memcpy( IncomingData, _IncomingData, Length );
                printf("[incoming]: %s\n",IncomingData);
                OutgoingData = HandleFau3Request((unsigned char *)IncomingData);
                lws_callback_on_writable_all_protocol( lws_get_context( WebsocketInstance ), lws_get_protocol( WebsocketInstance ) );
                printf("---------------\n");
            };
            break;

        case LWS_CALLBACK_SERVER_WRITEABLE: ;
            printf("[outgoing]: %s\n",OutgoingData);
            lws_write( WebsocketInstance, OutgoingData, strlen((const char *)OutgoingData), LWS_WRITE_TEXT );
            printf("---------------\n");
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
    { "Fau3VOMProto", Fau3Callback, 0, FAU3_BUFFER_BYTES },
    { NULL, NULL, 0, 0 } /* terminator */
};

int main( int argc, char *argv[] ){
    OpenObjectsStorage();
    struct lws_context_creation_info LwsInfo;
    memset( &LwsInfo, 0, sizeof(LwsInfo) );
    LwsInfo.port = FAU3_WS_PORT_CLIENT_ROUTER;
    LwsInfo.protocols = Protocols;
    LwsInfo.gid = -1;
    LwsInfo.uid = -1;
    struct lws_context *context = lws_create_context( &LwsInfo );
    while( 1 ){
        lws_service( context, /* timeout_ms = */ 100 );
    };
    lws_context_destroy( context );
    CloseObjectsStorage();
    return 0;
}
