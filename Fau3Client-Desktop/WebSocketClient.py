
import os
import time
import uuid
import json
import base64
import asyncio
import websockets
from datetime import datetime

FAU3_OK      =  0
FAU3_PONG    =  1
FAU3_STORED  =  3
FAU3_INVALID = -1
FAU3_SERROR  = -2

def Fau3Confirm(MessageUuid, StatusCode):
    return {
        "MessageType" : "Confirm",
        "MessageUuid" : MessageUuid,
        "Status"      : StatusCode,
    };

def Fau3Propogate(MessageUuid, RoomId, Payload):
    return {
        "MessageType"  : "Propogate",
        "MessageUuid"  : MessageUuid,
        "RoomId"       : RoomId,
        "Payload"      : Payload,
    };
def Fau3Subscribe(MessageUuid, RoomId, IsMonitorRequired = 5, IsPingRequired = 0):
    return {
        "MessageType"       : "Subscribe",
        "MessageUuid"       : MessageUuid,
        "RoomId"            : RoomId,
        "IsMonitorRequired" : IsMonitorRequired,
        "IsPingRequired"    : IsPingRequired,
    };
def Fau3Stream(Payload):
    return b"F3SB" + Payload + b"F3FN";

class Fau3WebSocketClient:
    def __init__(self, ServerUri, ClientId, DefaultChannel, IncomingQueue, OutgoingQueue, StoragePath):
        self.ServerUri = ServerUri;
        self.ClientId = ClientId;
        self.ClientRoomId = DefaultChannel;
        self.OutgoingQueue = OutgoingQueue;
        self.IncomingQueue = IncomingQueue;
        self.StoragePath = StoragePath;
        self.StreamBuffer = b'';
        self.StreamLastReceiveTime = 0;
    async def Fau3ServerConnect(self):
        try:
            async with websockets.connect(self.ServerUri,extra_headers={"fau3-client-id":self.ClientId}) as ClientWebSocket:
                Fau3WebSocketClient.Subscribe(self.OutgoingQueue, self.ClientRoomId);
                async def HandleIncomingMessages():
                    while True:
                        IncomingMessage = await ClientWebSocket.recv();
                        try:
                            IncomingMessageJson = json.loads(IncomingMessage);
                            if IncomingMessageJson:
                                if IncomingMessageJson["MessageType"] == "Ping":
                                    await ClientWebSocket.send(json.dumps(Fau3Confirm(IncomingMessageJson["MessageUuid"], FAU3_PONG)));
                                elif IncomingMessageJson["MessageType"] == "Playlist":
                                    await ClientWebSocket.send(json.dumps(Fau3Confirm(IncomingMessageJson["MessageUuid"], FAU3_STORED)))
                                    StorageChannelPath = self.StoragePath + f"/{self.ClientRoomId}";
                                    if not os.path.exists(StorageChannelPath):
                                        os.makedirs(StorageChannelPath);
                                    os.chdir(StorageChannelPath);
                                    for Record in IncomingMessageJson["Records"]:
                                        Filename = "In_"+datetime.now().strftime("%m_%d_%Y_%H_%M_%S_")+Record["ClientId"].lower().replace(":","");
                                        AbsoluteFilePath = f"{StorageChannelPath}/{Filename}";
                                        print(f"[NEW RECORD]: {AbsoluteFilePath}");
                                        with open(AbsoluteFilePath,"wb") as NewRecordFile:
                                            NewRecordFile.write(base64.b64decode(Record["Payload"].encode("utf-8")));
                                        self.IncomingQueue.put({"RecordFilename" : AbsoluteFilePath});
                                elif IncomingMessageJson["MessageType"] == "Monitor":
                                    await ClientWebSocket.send(json.dumps(Fau3Confirm(IncomingMessageJson["MessageUuid"], FAU3_OK)));
                                    self.IncomingQueue.put({"ConnectedClients" : IncomingMessageJson["ConnectedClients"]});
                                else:
                                    await ClientWebSocket.send(json.dumps(Fau3Confirm(IncomingMessageJson["MessageUuid"], FAU3_SERROR)));
                        # except json.JSONDecodeError:
                        except UnicodeDecodeError as Ex: # this is probably stream packet
                            if b"F3SB" == IncomingMessage[:4] and b"F3FN" == IncomingMessage[-4:]:
                                self.StreamBuffer += IncomingMessage[4:-4];
                                self.StreamLastReceiveTime = time.time();
                            continue;

                async def SendMessagesByExternalSignal():
                    while True:
                        if len(self.StreamBuffer) > 0 and time.time() - self.StreamLastReceiveTime > 2:
                            Filename = "In_Stream_"+datetime.now().strftime("%m_%d_%Y_%H_%M_%S");
                            AbsoluteFilePath = f"{self.StoragePath}/{Filename}";
                            print(f"[NEW RECORD]: {AbsoluteFilePath}");
                            with open(AbsoluteFilePath, "wb") as NewRecordFile:
                                NewRecordFile.write(self.StreamBuffer);
                            self.IncomingQueue.put({"RecordFilename" : AbsoluteFilePath});
                            self.StreamBuffer = b'';
                        if not self.OutgoingQueue.empty():
                            OutgoingMessage = self.OutgoingQueue.get(block=False);
                            if OutgoingMessage is None:
                                break;
                            if isinstance(OutgoingMessage, dict):
                                self.ClientRoomId = OutgoingMessage["RoomId"];
                                OutgoingMessage = json.dumps(OutgoingMessage);
                                print(f"[OutgoingMessage]: {OutgoingMessage}");
                            await ClientWebSocket.send(OutgoingMessage);
                        else:
                            await asyncio.sleep(0.2);
                            continue;

                TasksToExecute = [asyncio.create_task(HandleIncomingMessages()),asyncio.create_task(SendMessagesByExternalSignal())];
                AsyncGroup = asyncio.gather(*TasksToExecute);
                try:
                    await AsyncGroup;
                except Exception as Ex:
                    print(f'A task failed with: {Ex}, canceling all tasks');
                    for SingleTask in TasksToExecute:
                        SingleTask.cancel();
                    await asyncio.sleep(2);
        except:
            print("Could not connect to server:",datetime.now().strftime("%m/%d/%Y %H:%M:%S"));
            await asyncio.sleep(2);
    def Run(self):
        while True:
            asyncio.get_event_loop().run_until_complete(self.Fau3ServerConnect())
    @staticmethod
    def Propogate(Queue, RoomId, Payload):
        MessageUuid = str(uuid.uuid4());
        PayloadBase64 = base64.b64encode(Payload).decode("utf-8");
        Queue.put(Fau3Propogate(MessageUuid, RoomId, PayloadBase64));
    @staticmethod
    def Stream(Queue, Payload):
        Queue.put(Fau3Stream(Payload));
    @staticmethod
    def Subscribe(Queue, RoomId):
        MessageUuid = str(uuid.uuid4());
        Queue.put(Fau3Subscribe(MessageUuid, RoomId));

