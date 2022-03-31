#ifndef server_h
#define server_h

#include <string>
#include <map>

#include <Config.h>
#include <Serialize.h>

using namespace std;
  
int udp_server(void);
int tcp_server(void);
int server_console(void);

void setExit(const bool flag);
bool needExit(void);

void setRC_TCP(const int val, const string& text);
int getRC_TCP(string& text);

void setRC_UDP(const int val, const string& text);
int getRC_UDP(string& text);

void UDP_loop(int socketDesc = 0);

#endif // server_h