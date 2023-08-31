#!/bin/bash

function RunFau3Server {
    cd /home/fau3server/Desktop/Fau3Server;

    while [[ 1 ]]; do
        ./Fau3Server;
    done;
};

function RunFau3DesktopClient {
    cd /home/fau3server/Desktop/Fau3Client-Desktop
    while [[ 1 ]]; do
        ./GtkUserInterface.py;
        sleep 3;
    done;
};

RunFau3Server &
RunFau3DesktopClient

