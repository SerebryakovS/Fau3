#!/usr/bin/python3

import gi
import os
import time
import queue
import pyaudio
import threading
import subprocess
import multiprocessing as mp
from WebSocketClient import Fau3WebSocketClient;

gi.require_version('Gtk', '3.0');
from gi.repository import Gtk, GdkPixbuf,Gdk,GLib

ExecutionPath = os.getcwd();

class File(object):
    def __init__(self, Filename, PlaceHolder=False, Directory=True, Root=False, Empty=False):
        self.Filename = Filename
        self.PlaceHolder = PlaceHolder
        self.Directory = Directory
        self.Root = Root
        self.Empty = Empty

    def __str__(self):
        return 'File: name: {}, dir: {}, Empty: {}'.\
                format(self.Filename, self.Directory, self.Empty);

PlaceHolder = File('<should never be visible>', PlaceHolder=True);
EmptyDirectory = File('<Empty>', Empty=True);

class CircularImageButton(Gtk.EventBox):
    def __init__(self, IconName, ConnQueue):
        Gtk.EventBox.__init__(self);
        self.ConnQueue = ConnQueue;
        self.FPixBuf = GdkPixbuf.Pixbuf.new_from_file_at_size(ExecutionPath+"/FIcons/" + IconName, 100, 100);
        self.TPixBuf = GdkPixbuf.Pixbuf.new_from_file_at_size(ExecutionPath+"/TIcons/" + IconName, 100, 100);
        self.ImageToShow = Gtk.Image.new_from_pixbuf(self.FPixBuf);
        self.add(self.ImageToShow);

class RecordButton(CircularImageButton):
    def __init__(self, IconName, ConnQueue, RecordingChannel = 1, UseStreamMode = False):
        super().__init__(IconName, ConnQueue);
        self.IsRecording = False;
        self.UseStreamMode = UseStreamMode;
        self.RecordingChannel = RecordingChannel;
    def StartRecording(self, RecordingChannel):
        self.RecordingChannel = RecordingChannel;
        self.AudioHandler = pyaudio.PyAudio();
        self.Stream = self.AudioHandler.open(format = pyaudio.paInt16,
                                             channels = 1,
                                             rate = 8000,
                                             input = True,
                                             frames_per_buffer = 1024);
        self.RecorderedFrames = [];
        self.RecordThread = threading.Thread(target=self.RecordThreadFunction);
        self.IsRecording = True;
        self.RecordThread.start();
        self.ImageToShow.set_from_pixbuf(self.TPixBuf);

    def RecordThreadFunction(self):
        while self.IsRecording is True:
            self.RecorderedFrames.append(self.Stream.read(1024));

    def StopRecording(self):
        self.IsRecording = False;
        self.RecordThread.join();
        self.Stream.stop_stream();
        self.Stream.close();
        self.AudioHandler.terminate();
        if len(self.RecorderedFrames) > 0:
            Filename = "Out_"+str(int(time.time()));
            try:
                SpeexerProcess = subprocess.Popen(["./Speexer","enc", Filename],stdin=subprocess.PIPE);
                for Frame in self.RecorderedFrames:
                    SpeexerProcess.stdin.write(Frame);
                    SpeexerProcess.stdin.flush();
                SpeexerProcess.stdin.close();
                SpeexerProcess.wait();
                with open(Filename, "rb") as EncodedAudioFile:
                    if self.UseStreamMode == False:
                        Fau3WebSocketClient.Propogate(self.ConnQueue, self.RecordingChannel, EncodedAudioFile.read());
                    else:
                        Fau3WebSocketClient.Stream(self.ConnQueue, EncodedAudioFile.read());
            finally:
                os.remove(Filename);
        self.ImageToShow.set_from_pixbuf(self.FPixBuf);

class PlaybackButton(CircularImageButton):
    def __init__(self, IconName, ConnQueue):
        super().__init__(IconName, ConnQueue);
        self.ResetPlaying = False;
        self.IsPlaying = False;
    def StartPlaying(self):
        self.AudioHandler = pyaudio.PyAudio();
        self.Stream = self.AudioHandler.open(format = pyaudio.paInt16,
                                             channels = 1,
                                             rate = 8000,
                                             output = True,
                                             frames_per_buffer = 320);
        self.PlaybackThread = threading.Thread(target=self.PlaybackThreadFunction);
        self.IsPlaying = True;
        self.PlaybackThread.start();
        self.ImageToShow.set_from_pixbuf(self.TPixBuf);

    def PlaybackThreadFunction(self):
        while self.IsPlaying is True:
            try:
                NextPlaylistRecordName = self.ConnQueue.get(block=False);
                self.ResetPlaying = False;
                self.PlayingRecordName = NextPlaylistRecordName + ".temp";
                with open(NextPlaylistRecordName, "rb") as EncodedAudioFile:
                    SpeexerProcess = subprocess.Popen(["./Speexer","dec", self.PlayingRecordName],stdin=subprocess.PIPE);
                    SpeexerProcess.stdin.write(EncodedAudioFile.read());
                    SpeexerProcess.stdin.flush();
                    SpeexerProcess.stdin.close();
                    SpeexerProcess.wait();
                with open(self.PlayingRecordName, "rb") as PCMAudioFile:
                    while self.IsPlaying is True and self.ResetPlaying == False:
                        AudioChunk = PCMAudioFile.read(320);
                        if not AudioChunk:
                            break;
                        self.Stream.write(AudioChunk);
                os.remove(self.PlayingRecordName);
            except queue.Empty:
                time.sleep(0.5);
                continue;

    def StopPlaying(self):
        self.IsPlaying = False;
        self.PlaybackThread.join();
        self.Stream.stop_stream();
        self.Stream.close();
        self.AudioHandler.terminate();
        self.ImageToShow.set_from_pixbuf(self.FPixBuf);

class Fau3ClientGtkWrapper(Gtk.Window):
    def __init__(self,ServerUri, ClientId, IncomingQueue, OutgoingQueue, StoragePath, StoreAndReplay = False):
        Gtk.Window.__init__(self, title=self.__class__.__name__);
        self.Grid = Gtk.Grid();
        self.StoragePath = StoragePath;
        self.PlaylistQueue = queue.Queue();
        self.SelectedComboboxItem = "Channel1";
        self.IncomingQueue = IncomingQueue;
        self.OutgoingQueue = OutgoingQueue;
        self.override_background_color(Gtk.StateFlags.NORMAL, Gdk.RGBA(0.8, 0.8, 0.8, 1.0));

        # OriginalLogoPixBuf = Gtk.Image.new_from_file(ExecutionPath+"/logo.png").get_pixbuf();
        # ResizedLogoPixBuf = OriginalLogoPixBuf.scale_simple(20, 20, GdkPixbuf.InterpType.BILINEAR);
        # self.LogoImage = Gtk.Image.new_from_pixbuf(ResizedLogoPixBuf);
        # self.Grid.attach(self.LogoImage, 2, 0, 2, 1);

        self.PlaybackButton = PlaybackButton("play.png", self.PlaylistQueue);
        self.PlaybackButton.connect("button-press-event", self.OnPlaybackButtonClicked);
        self.PlaybackButton.StartPlaying();
        self.Grid.attach(self.PlaybackButton, 3, 2, 1, 3);

        self.WSClient = Fau3WebSocketClient(ServerUri, ClientId.upper(), self.SelectedComboboxItem, IncomingQueue, OutgoingQueue, StoragePath);
        mp.Process(target=self.WSClient.Run).start();
        Fau3WebSocketClient.Subscribe(self.OutgoingQueue, self.SelectedComboboxItem);

        threading.Thread(target=self.WebSocketIncomingMessagesReader).start()

        self.RecordButton = RecordButton("rec.png", self.OutgoingQueue);
        self.RecordButton.connect("button-press-event", self.OnRecordButtonPressed);
        self.RecordButton.connect("button-release-event", self.OnRecordButtonReleased);
        self.Grid.attach(self.RecordButton, 2, 2, 1, 3);

        ComboBoxListStore = Gtk.ListStore(str)
        for Item in [str(Idx) for Idx in range(1, 16)]:
            ComboBoxListStore.append([Item]);
        self.ComboBox = Gtk.ComboBox.new_with_model(ComboBoxListStore);
        self.ComboBox.set_wrap_width(len(ComboBoxListStore));
        self.ComboBox.set_active_iter(ComboBoxListStore.get_iter_first());
        ComboBoxTextRenderer = Gtk.CellRendererText();
        ComboBoxTextRenderer.set_property("xalign", 0.51);
        self.ComboBox.pack_start(ComboBoxTextRenderer, True);
        self.ComboBox.add_attribute(ComboBoxTextRenderer, "text", 0);
        self.ComboBox.connect("changed", self.OnSelectedLineChanged);
        self.Grid.attach(self.ComboBox, 2, 5, 2, 1);


        self.FileBrowserTreeModel = Gtk.TreeStore(object);
        if StoreAndReplay == True:
            self.FileBrowser = Gtk.Box(orientation=Gtk.Orientation.VERTICAL);
            ScrolledTree = Gtk.ScrolledWindow();
            self.FileBrowser.pack_start(ScrolledTree, True, True, 0);
            Tree = Gtk.TreeView(model=self.FileBrowserTreeModel);
            Tree.set_headers_visible(False);
            Tree.connect('row-expanded', self.OnRowExpanded);
            Tree.connect('row-activated', self.OnRowActivated);
            ScrolledTree.add(Tree);
            FilenameRenderer = Gtk.CellRendererText();
            FiletypeRenderer = Gtk.CellRendererPixbuf();
            FilenameColumn = Gtk.TreeViewColumn('file');
            FilenameColumn.pack_start(FiletypeRenderer, False);
            FilenameColumn.pack_start(FilenameRenderer, False);
            FilenameColumn.set_cell_data_func(FilenameRenderer, self.RenderFilename);
            FilenameColumn.set_cell_data_func(FiletypeRenderer, self.RenderFileTipePix);
            Tree.append_column(FilenameColumn);
            self.Grid.attach(self.FileBrowser, 3, 7, 3, 4);

        self.ClientsListStore = Gtk.ListStore(str, str, int);
        self.LanguageFilter = self.ClientsListStore.filter_new();
        self.ConnClientsTreeView = Gtk.TreeView(model=self.LanguageFilter);

        for Idx, CulumnTitle in enumerate(["Client Id", "Current Channel", "Alive"]):
            Renderer = Gtk.CellRendererText();
            Column = Gtk.TreeViewColumn(CulumnTitle, Renderer, text=Idx);
            self.ConnClientsTreeView.append_column(Column);
        self.ConnClientsScrollableTreelist = Gtk.ScrolledWindow();
        self.ConnClientsScrollableTreelist.set_vexpand(True);
        self.ConnClientsScrollableTreelist.add(self.ConnClientsTreeView);

        if StoreAndReplay == True:
            self.Grid.attach(self.ConnClientsScrollableTreelist, 0, 7, 3, 4);
        else:
            self.Grid.attach(self.ConnClientsScrollableTreelist, 1, 7, 4, 4);

        self.Grid.set_column_homogeneous(True);
        self.Grid.set_row_homogeneous(True);
        self.Grid.set_column_spacing(10);
        self.Grid.set_row_spacing(10);
        self.Grid.set_halign(Gtk.Align.CENTER);
        self.Grid.set_valign(Gtk.Align.CENTER);

        self.add(self.Grid);

    def WebSocketIncomingMessagesReader(self):
        while True:
            WSMessage = self.IncomingQueue.get();
            if "RecordFilename" in WSMessage.keys():
                self.PlaylistQueue.put(WSMessage["RecordFilename"]);
                self.RefreshTreeView();
            elif "ConnectedClients" in WSMessage.keys():
                self.UpdateConnectedClients(WSMessage["ConnectedClients"]);


    def UpdateConnectedClients(self, ConnectedClients):
        self.ClientsListStore.clear();
        for ClientInfoDict in ConnectedClients:
            ClientInfo = (ClientInfoDict["ClientId"],ClientInfoDict["RoomId"],ClientInfoDict["AliveTime"]);
            self.ClientsListStore.append(list(ClientInfo));

    def OnRowActivated(self, Treeview, Path, Column):
        Iter = Treeview.get_model().get_iter(Path);
        if Iter:
            File = Treeview.get_model()[Iter][0];
            if self.PlaybackButton.IsPlaying == True:
                self.PlaybackButton.ResetPlaying = True;
                self.PlaybackButton.ConnQueue.put(File.Filename);

    def RefreshTreeView(self):
        self.FileBrowserTreeModel.clear();
        self.AddDir(self.StoragePath);

    def OnRowExpanded(self, widget, TreeIter, Path):
        CurrentDir = self.FileBrowserTreeModel[TreeIter][0];
        PlaceHolderIter = self.FileBrowserTreeModel.iter_children(TreeIter);
        if not self.FileBrowserTreeModel[PlaceHolderIter][0].PlaceHolder:
            return;
        Paths = os.listdir(CurrentDir.Filename);
        Paths.sort();
        if len(Paths) > 0:
            for ChildPath in Paths:
                self.AddDir(os.path.join(CurrentDir.Filename, ChildPath), TreeIter);
        else:
            self.FileBrowserTreeModel.append(TreeIter, [EmptyDirectory]);
        self.FileBrowserTreeModel.remove(PlaceHolderIter);


    def RenderFileTipePix(self, Column, Renderer, Model, TreeIter, user_data):
        RenderingFile = Model[TreeIter][0];
        if RenderingFile.Empty:
            Renderer.set_property('stock_id', None)
        elif RenderingFile.Directory:
            Renderer.set_property('stock-id', Gtk.STOCK_OPEN);
        else:
            Renderer.set_property('stock-id', Gtk.STOCK_FILE);

    def RenderFilename(self, Column, Renderer, Model, TreeIter, user_data):
        RenderingFile = Model[TreeIter][0]
        if RenderingFile.Root:
            Label = RenderingFile.Filename
        else:
            Label = os.path.basename(RenderingFile.Filename)
        Hidden = os.path.basename(RenderingFile.Filename)[0] == '.'
        Label = GLib.markup_escape_text(Label)
        if Hidden:
            Label = '<i>' + Label + '</i>'
        Renderer.set_property('markup', Label)

    def AddDir(self, dir_name, Root=None):
        is_Root = Root == None
        if os.path.isdir(dir_name):
            TreeIter = self.FileBrowserTreeModel.append(Root, [File(dir_name, Root=is_Root)])
            self.FileBrowserTreeModel.append(TreeIter, [PlaceHolder])
        else:
            self.FileBrowserTreeModel.append(Root, [File(dir_name, Root=is_Root, Directory=False)])

    def OnRecordButtonPressed(self, Widget, Event):
        if self.PlaybackButton.IsPlaying is True:
            self.PlaybackButton.StopPlaying();
            self.RecordButton.StartRecording(self.SelectedComboboxItem);

    def OnRecordButtonReleased(self, Widget, Event):
        self.RecordButton.StopRecording();
        self.PlaybackButton.StartPlaying();

    def OnPlaybackButtonClicked(self, Widget, Event):
        if self.PlaybackButton.IsPlaying is False:
            self.PlaybackButton.StartPlaying();
        elif self.PlaybackButton.IsPlaying is True:
            self.PlaybackButton.StopPlaying();

    def OnSelectedLineChanged(self, ComboBox):
        SelectedItem = ComboBox.get_active_iter();
        if SelectedItem is not None:
            Model = ComboBox.get_model();
            self.SelectedComboboxItem = "Channel"+Model[SelectedItem][0];
            Fau3WebSocketClient.Subscribe(self.OutgoingQueue, self.SelectedComboboxItem);
            print("SelectedComboboxItem changed:", self.SelectedComboboxItem);

if __name__ == "__main__":
    StoragePath = os.getcwd()+"/IncMessages";
    if not os.path.exists(StoragePath):
        os.makedirs(StoragePath);
    Fau3ClientId  = "08:00:27:8f:9e:28";
    Fau3ServerUri = "ws://127.0.0.1:8901";
    Window = Fau3ClientGtkWrapper(Fau3ServerUri,Fau3ClientId,mp.Queue(),mp.Queue(),StoragePath);
    Window.connect("destroy", Gtk.main_quit);
    # Window.fullscreen();
    Window.show_all();
    Window.AddDir(Window.StoragePath);
    Gtk.main();
