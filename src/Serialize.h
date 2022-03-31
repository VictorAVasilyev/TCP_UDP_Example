#ifndef serialize_h
#define serialize_h

#include <string>
#include <vector>
#include <map>

using namespace std;

#define PACKAGED_NUMBER_LEN 4 // use 4 characters for package Id
#define PACKAGE_HEADER_LEN 3*PACKAGED_NUMBER_LEN // header contains 3 numbers: clientId, numBlocks, blockId
#define SERVER_ID 1

struct Package {
  int clientId;
  int numBlocks;
  int blockId;
  string data;
  
  Package();
  void clear();
};

struct ReceivedData {
  Package output;
  int numPackagesToReceive;
  int numPackagesReceived;
  string receivedStr;
  
  ReceivedData();
  void clear();
};

bool findClientFinishedTransfer(const map<int, ReceivedData>& dataReceivedFromClient, int& clientId);

int serialize(const int bufferLength, const int clientId, const string& input, vector<string>& serializedOutput);

int deserialize(const int bufferLength, const string& input, Package& output);

#endif // serialize_h