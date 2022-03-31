#ifndef config_h
#define config_h

#define PORT_UDP 8888
#define PORT_TCP 8889
#define UDP_BUF_LEN 16
#define TCP_BUF_LEN 16
#define SERVER "127.0.0.1"
#define MAX_CLIENTS 30 // used for TCP; size of connections pool; TODO: we can make it variable 
#define MAX_BACKLOG 128 // used for TCP listen()
#define TIMEOUT_SEC 1
#define TIMEOUT_MICROSEC 0
#define TIMEOUT_HANG 10 // seconds

#endif // config_h