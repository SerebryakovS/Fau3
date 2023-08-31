all: Fau3Server Fau3Client

Fau3Server: server.c
	gcc fau3vomlib.c clientsHandler.c objectsStorage.c -g -Wall -I/usr/include/json-c/ -L/usr/lib/libjson-c.so $< -o $@ `pkg-config libwebsockets --libs --cflags` -ljson-c -lhiredis -pthread
clean:
	rm -f Fau3Server
