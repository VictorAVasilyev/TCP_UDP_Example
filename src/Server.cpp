#include <iostream>
#include <future>
#include <chrono>
#include <thread>
#include <poll.h>
#include <unistd.h>

#include <Server.h>

using namespace std;

// global variables
bool exitFlag;
int rcTCP;
int rcUDP;
string errorTCP;
string errorUDP;
mutex m_exit;
mutex m_TCP;
mutex m_UDP;
mutex m_console;

int main(void)
{
  {
    lock_guard<mutex> lock(m_console);
    cout << "Server started.\n";
  }
  thread thread_udp{udp_server};
  thread thread_tcp{tcp_server};
  thread thread_console{server_console};

  // check in a loop if it's necessary to exit or some errors happened
  bool isRC_TCP_printed = false;
  bool isRC_UDP_printed = false;
  while(true) {
    
    this_thread::sleep_for(chrono::milliseconds(1000));

    bool flagBreak = false;
    if (needExit()) {
        flagBreak = true;
    }
    string textTCP;
    int rcTCP = getRC_TCP(textTCP);
    if ((rcTCP != 0) && (! isRC_TCP_printed)) {
        isRC_TCP_printed = true;
        lock_guard<mutex> lock(m_console);
        cout << "Error in TCP server: "<<textTCP<<endl;
    }
    string textUDP;
    int rcUDP = getRC_UDP(textUDP);
    if ((rcUDP != 0) && (! isRC_UDP_printed)) {
        isRC_UDP_printed = true;
        lock_guard<mutex> lock(m_console);
        cout << "Error in UDP server: "<<textUDP<<endl;
    }
    if ((rcTCP != 0) && (rcUDP != 0)) {
        setExit(true);
        flagBreak = true;
        lock_guard<mutex> lock(m_console);
        cout << "Both UDP and TCP servers failed, exit\n";
    }
    if (flagBreak) {
      break;
    }
  }
    
  thread_udp.join();
  thread_tcp.join();
  thread_console.join();

  return 0;
}

int server_console(void)
{
  {
    lock_guard<mutex> lock(m_console);
    cout << "Type \'exit\' to stop: "<<endl;
  }
  string input;
  {
    struct pollfd pfd = { STDIN_FILENO, POLLIN, 0 };
    int ret = 0;
    while(ret == 0) {
      if (needExit()) {
        // exit if both TCP and UDP threads failed - there's nothing to do any more, need to fix network problems and restart server.
        break;
      }
      // use poll to avoid thread lock by getline
      ret = poll(&pfd, 1, 1000);
      if(ret == 1) { // there is something to read
        getline(cin, input); 
      }
    }
  }
  int res = input.compare("exit");
  if (0 == res)
  {
    setExit(true);
  }
  return 0;
}

void setExit(const bool flag)
{
  lock_guard<mutex> lock(m_exit);
  exitFlag = flag;
}

bool needExit(void)
{
  bool flag = true;
  {
    lock_guard<mutex> lock(m_exit);
    flag = exitFlag;
  }
  return flag;
}

void setRC_TCP(const int val, const string& text)
{
  lock_guard<mutex> lock(m_TCP);
  rcTCP = val;
  errorTCP = text;
}

int getRC_TCP(string& text)
{
  int val = 0;
  text = "";
  {
    lock_guard<mutex> lock(m_TCP);
    val = rcTCP;
    text = errorTCP;
  }
  return val;
}

void setRC_UDP(const int val, const string& text)
{
  lock_guard<mutex> lock(m_UDP);
  rcUDP = val;
  errorUDP = text;
}

int getRC_UDP(string& text)
{
  int val = 0;
  text = "";
  {
    lock_guard<mutex> lock(m_UDP);
    val = rcUDP;
    text = errorUDP;
  }
  return val;
}





