#include <iostream>
#include <sstream>
#include <fstream>
#include <chrono>
#include <thread>
#include <cstring>

#include <Server.h>
#include <Test.h>

using namespace std;

const string endtest = "endtest";
const string badRefFile = "badRefFile";

string dataFile;
bool timeoutExpired = false;
vector<string> sentData;
bool testOk = false;

int main(int argc, char *argv[]) 
{
  
  sentData.clear();
  timeoutExpired = false;
  testOk = false;
  dataFile = "";
  vector<string> argList(argv+1, argv + argc);
  if (!argList.empty()) {
    dataFile = argList[0];
  }
  
  //TODO: current implementation of test suite checks if packages are correct.
  // It does not test multiple connections.
  // Also test of TCP server not implemented.
  try{
    cout << "Test started\n";
    
    UDP_loop(0);
    
  } catch (runtime_error& e) {
    const string& reason = e.what();
    if (reason == endtest) {
      if (testOk) {
        cout << "Test finished, status = OK"<<endl;
      }
      else {
        cout << "Test finished, status = FAILED"<<endl;
      }
    }
    else if (reason == badRefFile) {
      cout << "Test not started due to bad reference file"<<endl;
    }
  }
  
  return 0;
}

bool timeout_test(void)
{
  return timeoutExpired;
}

ssize_t recv_test(int sockfd, void *buf, size_t len, int flags)
{
  //TODO
  return 0;
}

ssize_t recvfrom_test(int sockfd, void *buf, size_t len, int flags,
                 struct sockaddr *src_addr, socklen_t *addrlen)
{
  static int numLineRead = 0;
  string line;
  ifstream testfile(dataFile);
  if (!testfile.is_open()) {
    throw runtime_error(badRefFile);
  }
  else
  {
    int currentLine = 0;
    while ( currentLine <= numLineRead )
    {
      getline (testfile,line); //TODO: use seek
      currentLine++;
    }
    if (testfile.eof()) {
      // test execution finished, exit
      testfile.close();
      testFinished();
      throw runtime_error(endtest); //TODO: exception is not a good way for normal program termination, need something else
    }
    numLineRead++;
    testfile.close();
  }
  
  if (line.length() > 2) {
    if ('w' == line[0]) { // 'w' means wait 
      // read time and sleep
      stringstream ssRead;
      ssRead << line;
      string temp;
      int time = 0;
      bool timeOk = false;
      while (!ssRead.eof()) {
          ssRead >> temp; 
          if (stringstream(temp) >> time) {
              timeOk = true;
          }
          temp = "";
      }
      if (timeOk) {
        if (time > TIMEOUT_SEC) {
          timeoutExpired = true;
        }
        //TODO: this implementation does not respect SO_RCVTIMEO and SO_SNDTIMEO
        int sleepMilliseconds = time*1000;
        this_thread::sleep_for(chrono::milliseconds(sleepMilliseconds));
      }
      return -1;
    }
    else if ('b' == line[0]) { // 'b' means block of data
      char* buf_char = (char*)buf;
      const char* line_c = line.c_str();
      strncpy(&buf_char[0], &line_c[2], len);
      //TODO: fill src_addr
      return len;
    }
  }
}

ssize_t send_test(int sockfd, const void *buf, size_t len, int flags)
{
  //TODO
  return 0;
}

ssize_t sendto_test(int sockfd, const void *buf, size_t len, int flags,
               const struct sockaddr *dest_addr, socklen_t addrlen)
{
  char* buf_char = (char*)buf;
  string str(buf_char);
  sentData.push_back(str);
  //TODO: check dest_addr
  
}
 
int testFinished()
{
  vector<string> referenceLines;
  ifstream testfile(dataFile);
  if (!testfile.is_open()) {
    throw runtime_error(badRefFile);
  }
  else
  {
    while ( !testfile.eof() )
    {
      string line;
      getline (testfile,line);
      if (line.length() > 2) {
        if ('r' == line[0]) { // 'r' means reference
          string ref = line.substr(2, line.length()-2);
          referenceLines.push_back(ref);
        }
      }
    }
    testfile.close();
  }
  if (sentData.size() != referenceLines.size()) {
    testOk = false;
    return 0;
  }
  testOk = true;
  for (size_t i = 0; i < sentData.size(); i++) {
    const string& ref = referenceLines[i];
    const string& actual = sentData[i];
    if (ref != actual) {
      testOk = false;
      break;
    }
  }

  return 0;
}
