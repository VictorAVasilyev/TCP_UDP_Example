#include <future>
#include <sstream>

#include <Config.h> // package length
#include <Serialize.h>

using namespace std;

Package::Package()
: clientId(0)
, numBlocks(0)
, blockId(0)
{}

void Package::clear() {
  clientId = 0;
  numBlocks = 0;
  blockId = 0;
  data = "";
}

ReceivedData::ReceivedData()
: numPackagesToReceive(0)
, numPackagesReceived(0)
{}

void ReceivedData::clear()
{
  receivedStr = "";
  numPackagesToReceive = 0;
  numPackagesReceived = 0;
  output.clear();
}

bool findClientFinishedTransfer(const map<int, ReceivedData>& dataReceivedFromClient, int& clientId)
{
  clientId = 0;
  for (auto it = dataReceivedFromClient.cbegin(); it != dataReceivedFromClient.cend(); it++) {
    const ReceivedData& receivedData = it->second;
    if (receivedData.numPackagesReceived >= receivedData.numPackagesToReceive) {
      clientId = receivedData.output.clientId;
      return true;
    }
  }
  return false;
}

int serialize(const int bufferLength, const int clientId, const string& input, vector<string>& serializedOutput)
{
    serializedOutput.clear();
    if (bufferLength < PACKAGE_HEADER_LEN) {
        string what = "Too small buffer";
        throw runtime_error(what);
    }
    
    size_t inputLength = input.length();
    int blockLen = bufferLength-1 - PACKAGE_HEADER_LEN;
    bool hasTail = ((inputLength % blockLen) > 0);
    int numBlocks = inputLength / blockLen;
    if (hasTail) {numBlocks++;}
    
    mutex m;
    lock_guard<mutex> lock{m};
    string clientIdStr;
    string numBlocksStr;
    stringstream ss;
    ss << clientId;
    ss >> clientIdStr;
    if (clientIdStr.length() > PACKAGED_NUMBER_LEN) {
        string what = "Too large client Id";
        throw runtime_error(what);
    }
    string clientIdFormatted(PACKAGED_NUMBER_LEN, ' ');
    clientIdFormatted.replace(PACKAGED_NUMBER_LEN - clientIdStr.length(), PACKAGED_NUMBER_LEN, clientIdStr);
    stringstream ss2;
    ss2 << numBlocks;
    ss2 >> numBlocksStr;
    if (numBlocksStr.length() > PACKAGED_NUMBER_LEN) {
        string what = "Too large number of packages to send";
        throw runtime_error(what);
    }
    string numBlocksFormatted(PACKAGED_NUMBER_LEN, ' ');
    numBlocksFormatted.replace(PACKAGED_NUMBER_LEN - numBlocksStr.length(), PACKAGED_NUMBER_LEN, numBlocksStr);
    size_t strPos = 0;
    int blockId = 0;
    do {
        string nextStr(bufferLength,0);
        nextStr.replace(0, clientIdFormatted.length(), clientIdFormatted);
        nextStr.replace(clientIdFormatted.length(), numBlocksFormatted.length(), numBlocksFormatted);
        string strBlockId;
        //ss.flush(); //TODO: bug: cannot reuse stringstream
        stringstream ss3;
        ss3 << blockId;
        ss3 >> strBlockId;
        if (strBlockId.length() > PACKAGED_NUMBER_LEN) {
            string what = "Too large number of packages to send";
            throw runtime_error(what);
        }
        string blockIdFormatted(PACKAGED_NUMBER_LEN, ' ');
        blockIdFormatted.replace(PACKAGED_NUMBER_LEN - strBlockId.length(), PACKAGED_NUMBER_LEN, strBlockId);
        nextStr.replace(clientIdFormatted.length() + numBlocksFormatted.length(), numBlocksFormatted.length() /*+ blockIdFormatted.length()*/, blockIdFormatted);
        string inputSubstr = input.substr(strPos, blockLen);
        nextStr.replace(clientIdFormatted.length() + numBlocksFormatted.length() + blockIdFormatted.length(), 
                        bufferLength-(clientIdFormatted.length() + numBlocksFormatted.length() + blockIdFormatted.length())-1, 
                        inputSubstr);
        serializedOutput.push_back(nextStr);
        blockId++;
        strPos += blockLen;
        
    } while(strPos < input.length());
    
    return 0;
}

int deserialize(const int bufferLength, const string& input, Package& output)
{
    output.clear();
    int blockLen = bufferLength - PACKAGE_HEADER_LEN;
    
    mutex m;
    lock_guard<mutex> lock{m};
    stringstream ss;

    if (input.length() <= PACKAGE_HEADER_LEN ) {
      // skip bad package
      return 1;
    }
    
    string clientIdSubstr = input.substr(0, PACKAGED_NUMBER_LEN);
    int clientId = 0;
    //ss.flush(); //TODO: bug: cannot reuse stringstream
    ss << clientIdSubstr;
    ss >> clientId;
    output.clientId = clientId;
    
    stringstream ss2;
    string numBlocksSubstr = input.substr(PACKAGED_NUMBER_LEN, PACKAGED_NUMBER_LEN);
    int numBlocks = 0;
    //ss.flush();
    ss2 << numBlocksSubstr;
    ss2 >> numBlocks;
    output.numBlocks = numBlocks;

    
    string blockIdSubstr = input.substr(2*PACKAGED_NUMBER_LEN, PACKAGED_NUMBER_LEN);
    int blockId = 0;
    //ss.flush();
    stringstream ss3;
    ss3 << blockIdSubstr;
    ss3 >> blockId;
    output.blockId = blockId;

    string dataSubstr = input.substr(PACKAGE_HEADER_LEN, input.length() - PACKAGE_HEADER_LEN);
    output.data = dataSubstr;
    
    return 0;
}
