#!/bin/bash

gcc Client.c -o client -l sqlite3 -pthread -lm
gcc Server.c -o server -l sqlite3  -pthread
gcc ffa_server.c -o ffa -l sqlite3  -pthread
gcc solo_server.c -o solo -l sqlite3  -pthread
gcc duo_server.c -o duo -l sqlite3  -pthread

gnome-terminal -- ./server
gnome-terminal -- ./ffa
gnome-terminal -- ./solo
gnome-terminal -- ./duo

for i in {1..6}
do
    gnome-terminal -- ./client
done
