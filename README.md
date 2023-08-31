
<img src="logo.png" alt="logo" width="100"/>

--------------------------------------------------------------------------------------------------------------------------

## FAU3VOM-D API ( default )

Each client should insert additional HTTP header while establishing new connection called: fau3-client-id: UniqueClientId, which is used to register client.
Protocol mechanism requires that every received packet need to be confirmed otherwise the last will be retransmitted K times. According to current specification each client should cache last N received messages(except 'Ping' type messages) in order to prevent playlist data dublicates.

### Ping
This is optional message, which is periodically sent by server if previously this behaviour had been requested in client subscription.
It's general purpose is to handle connection heartbits. If client doesn't response on it then server deletes it from active list:
```
    {
        "MessageType"  : "Ping",
        "MessageUuid"  : "..."
    }
```

### Monitor
This is optional message, which is periodically sent by server if previously this behaviour had been requested in client subscription:
```
    {
        "MessageType"  : "Monitor",
        "MessageUuid"  : "...",
        "ConnectedClients" : [
            {
                "ClientId"  : "...",
                "RoomId"    : "...",
                "AliveTime" : "..."
            },
        ]
    }

```
### Confirm
Every message(including ping) need to be confirmed by the following:
```
    {
        "MessageType"  : "Confirm",
        "MessageUuid"  : "...",
        "Status"       : <FAU3_STATUS_CODE>
    }
```
### Propogate
Clients sends recordeded and encoded message:
```
    {
        "MessageType"  : "Propogate",
        "MessageUuid"  : "...",
        "RoomId"       : "...",
        "Payload"      : "..." // base64 encoded message
    }
```
### Subscribe
Client subsribes for messages from specific room:
```
    {
        "MessageType"        : "Subscribe",
        "MessageUuid"        : "...",
        "RoomId"             : "...",
        "IsMonitorRequired"  : "...", // in seconds (optional parameter)
        "IsPingRequired"     : "..."  // in seconds (optional parameter)
    }
```
if IsMonitorRequired | IsPingRequired are negative, it's disabled
### Playlist
After successful subscription new messages will be received by client:
```
    {
        "MessageType"   : "Playlist",
        "MessageUuid"   : "...",
        "Records"  : [
            {
                "ClientId" : "...",
                "Payload"  : "..." // base64 encoded message
            },
            ...
        ]
    }
```
### Stream
This type of non-confirmation required packets which should be marked with specific way('F3SB' mark at the beginning of the stream and 'F3FN' at the end).

Next status codes declared for D version:

* **FAU3_PONG**    - Ping response, connection confirmation
* **FAU3_MONED**   - Monitor response, connection confirmation
* **FAU3_OK**      - General confirmation response code
* **FAU3_INVALID** - Invalid packet received, some fields does not presented in packet
* **FAU3_STORED**  - Playlist or single message(over 'Propogate' call) has been received and stored
* **FAU3_SERROR**  - Internal server error


config domain
        option name 'fau3vomdMain'
        option ip '10.10.11.156'

--------------------------------------------------------------------------------------------------------------------------

Debian 11 installation:
```
sudo apt-get install libwebsockets-dev libjson-c-dev redis-server libhiredis-dev
```
Autostart installation:
```
cd /etc/xdg/autostart
sudo nano fau3server.desktop
```
before using Desktop client:
```
gcc Speexer.c -o Speexer -lspeex
```
and on server:
```
make clean && make Fau3Server
```



FAU3VOM-E API ( extended )

Здесь шейрить клиентские sessionUID и их настройки(по подпискам), тоже придется

Base flow:
 ⁃ Router starts acting as P2P-server ( router extended mode, ERouter )
 ⁃ ERouter opens additional WS-connection with another ERouter
 ⁃ ERouters synchronize received events with each other
 ⁃ Client and router communication organized according to FAU3VOM-D

Fau3VOMEProto

1. Scan for servers in network (Fau3BatScan is the default interface)
2. Send request to another server to get information about it's state:
{
    "MessageType" : "State",
    "ConnectedClients" : [
        "ClientId" : "...",
        "RoomId" : "..."
    ],
}
3. Messages tranmitted in standard Playlist or Stream manner





Должен быть реализован алгоритм, который по имеющимся данным о сегменте сети(и о списке extra-серверов, которые получаются по топологии) смог определить для всех нод одинакового координатора. То есть должны быть интегрированы начала кастомного P2P(router-router API) для недопущения алгоритмической ошибки и включения нескольких серверов на одном сегменте

Main(default) flow:
 ⁃ Router get’s full available topology map(the way of getting topology depends on application)

/fau3vom/GetExtraServersList ( router -> server )
Received list is used in cases when default server is unavailable for some reason. In this situation each router tries to connect with other one(getting one by one from top of received list). Fo
 • Each router from list knows about it’s own priority level and switches to router if higher by priority router is unavailable
 • If no one from ExtraServers list is available, then router switches to server mode
 • Each router switched to server mode regullaty check higher priority routers and default server in order to restore defaults as soon as possible


FAU3VOM-F API ( full )






Client software notes:

- Every speex encoded audio chunk stored in NVS before transmition. The maximum key length provided by ESP-IDF is currently 15 characters. For the identification of chunk in Fau3Client used base meta strcture UUID(v4)'s last 12 bytes plus serial number. So, we have the range within: [0-999], and: 999 x 20[msec] = 19980[msec], almost 20[sec] of audio per single outgoing message.


TODO:
1. Подумать, сейчас идет привязка к соединению, но не к ClientId, возможно стоит это пересмотреть

