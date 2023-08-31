all: fau3vom

fau3vom: server.c fau3vomlib.c objectsStorage.c fau3vomlib.h
# 	gcc server.c fau3vomlib.c objectsStorage.c -g -Wall -I$(STAGING_DIR)/usr/include/json-c/ -L/usr/lib/libjson-c.so $< -o $@ `pkg-config libwebsockets --libs --cflags` -ljson-c -lhiredis
	$(TARGET_CC) -fhonour-copts $(TARGET_CFLAGS) $(TARGET_LDFLAGS) -I$(STAGING_DIR)/usr/include/json-c/ -o $@ $^ -lwebsockets -ljson-c -lhiredis
clean:
	rm -f fau3vom


