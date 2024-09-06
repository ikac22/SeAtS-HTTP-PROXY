MAKE=make
CC_m=g++
CFLAGS_m= -I./ -I./SeAtS/include -g -pg -Wall -Werror -Wextra -O0 

LDFLAGS_m= -L./ -lseats -lcrypto -lssl 

all: client server

lib: proxy_parse.c
	$(MAKE) -C SeAtS lib
	$(CC_m) $(CFLAGS_m) -o proxy_parse.o -c proxy_parse.c 

client: lib client_proxy.cpp 
	$(CC_m) $(CFLAGS_m) -o client_proxy.o -c client_proxy.cpp
	$(CC_m) $(LDFLAGS_m) -o client_proxy proxy_parse.o client_proxy.o 

server: lib server_proxy.cpp 
	$(CC_m) $(CFLAGS_m) -o server_proxy.o -c server_proxy.cpp
	$(CC_m) $(LDFLAGS_m) -o server_proxy proxy_parse.o server_proxy.o 
	
clean:
	rm -f proxy *.o
