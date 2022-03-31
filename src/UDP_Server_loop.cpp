#include<string.h> //memset
#include<arpa/inet.h>
#include <unistd.h> // close
#include <stdexcept>
#include <algorithm>
#include <chrono>
#include <set>

#ifdef TEST
#include <../test/Test.h>
#endif

#include <Serialize.h>
#include <Math.h>
#include <Server.h>

using namespace std;

void UDP_loop(int socketDesc)
{
    char buffer[UDP_BUF_LEN]{0};
    int res = 0;
    int rc = 0;
    int recv_len = 0;
    struct sockaddr_in si_client{0};
    socklen_t slen = sizeof(si_client);
    map<int, ReceivedData> dataReceivedFromClient;
    // For timeBeginReceiving and clientAddress:
    // Key is clientId. Only clients which have not finished data transfer are stored in this map.
    map<int, chrono::time_point<chrono::high_resolution_clock>> timeBeginReceiving; 
    map<int, sockaddr_in> clientAddress;

    while(true)
    {

        int blockLen = UDP_BUF_LEN - PACKAGE_HEADER_LEN-1;
        bool dataReceived = false;
        int clientIdReceived = 0;
        bool blockOk = true;
        set<int> clientsBeyondTimeout;

        do {
            bool allClientsHang = false;
            memset(&buffer[0], 0, UDP_BUF_LEN);
            //try to receive some data
            while(true) {
              // check if 'exit' was entered by server admin
#ifdef TEST
              bool goExit = false;
#else
              bool goExit = needExit();
#endif
              if (goExit) {
                  rc = 0;
                  string what = "exit";
                  throw runtime_error(what);
              }

              bool timeoutExpired = false;
#ifdef TEST
              recv_len = recvfrom_test(socketDesc, buffer, UDP_BUF_LEN, 0, (struct sockaddr *) &si_client, &slen);
              timeoutExpired = timeout_test();
#else
              recv_len = recvfrom(socketDesc, buffer, UDP_BUF_LEN, 0, (struct sockaddr *) &si_client, &slen);
              timeoutExpired = ((EAGAIN == errno) || (EWOULDBLOCK == errno));
#endif
              if (-1 == recv_len)
              {
                if(timeoutExpired) {
                  // timeout expired - keep listening
                  int numClientsToListen = 0;
                  auto nowTime = std::chrono::high_resolution_clock::now();
                  for (auto it = timeBeginReceiving.begin(); it != timeBeginReceiving.end(); it++) {
                    std::chrono::duration<double> diff = nowTime - it->second;
                    if (diff.count() > TIMEOUT_HANG) {
                      // if client has started data transfer, not completed it,
                      // and does not send data for a long time, then something is wrong
                      clientsBeyondTimeout.insert(it->first);
                    }
                    else {
                      numClientsToListen++;
                    }
                  }
                  if ((0 == numClientsToListen) && (clientsBeyondTimeout.size() > 0)) {
                    // there are clients trying to send data but no packages received by server - network error
                    allClientsHang = true;
                    break;
                  } else {
                    // timeout expired - keep listening
                    continue;
                  }
                }
                else
                {
                  // error - exit
                  rc = 5;
                  string what = "recvfrom failed";
                  throw runtime_error(what);
                }
              }
              else
              {
                // data received - exit from listen loop and run business logic
                break;
              }
            }
            if (allClientsHang) {
              break;
            }
            // data received - deserialize it
            string input(buffer);
            Package output;
            int deserializeOk = deserialize(UDP_BUF_LEN, input, output);
            if (deserializeOk != 0) {
              continue; 
            }
            auto nowTime = std::chrono::high_resolution_clock::now();
            if (timeBeginReceiving.find(output.clientId) == timeBeginReceiving.end()) {
              timeBeginReceiving[output.clientId] = nowTime;
              clientAddress[output.clientId] = si_client;
            }
            int numClientsToListen = 0;
            for (auto it = timeBeginReceiving.begin(); it != timeBeginReceiving.end(); it++) {
              if (it->first != output.clientId){
                std::chrono::duration<double> diff = nowTime - it->second;
                if (diff.count() > TIMEOUT_HANG) {
                  clientsBeyondTimeout.insert(it->first);
                }
                else {
                  numClientsToListen++;
                }
              }
            }
            if ((0 == numClientsToListen) && (clientsBeyondTimeout.size() > 0)) {
              break;
            }
            
            if (output.numBlocks <= 0) {
              blockOk = false;
            }
            if ((output.blockId < 0) || (output.blockId > output.numBlocks))
            {
              blockOk = false;
            }
            auto itDatReceived = dataReceivedFromClient.find(output.clientId);
            if (itDatReceived != dataReceivedFromClient.end()) {
              const ReceivedData& receivedData = itDatReceived->second;
              if (output.numBlocks != receivedData.numPackagesToReceive) {
                blockOk = false;
              }
            } else {
                // first valid package reveiced - initialize necessary data
                ReceivedData receivedData;
                receivedData.output = output;
                receivedData.numPackagesToReceive = output.numBlocks;
                receivedData.receivedStr = string(blockLen*receivedData.numPackagesToReceive+1, 0);
                dataReceivedFromClient[output.clientId] = receivedData;
                itDatReceived = dataReceivedFromClient.find(output.clientId);
            }
            
            if (!blockOk) {
              // stop data transfer for current client, send error message
              // TODO: what to do if further data from this client input will be received?
              // In current implementation the new error messages will be sent to client,
              // so client can receive many messages from one error.
              clientIdReceived = output.clientId;
              break;
            }

            ReceivedData& receivedData = itDatReceived->second;
            // insert data substring into proper position according to blockId, because by UDP packages can be received in any order
            receivedData.receivedStr.replace(output.blockId*blockLen, min(blockLen, (int)output.data.length()), output.data);
            receivedData.numPackagesReceived++;
            // check if data transfer was finished for one of clients
            dataReceived = findClientFinishedTransfer(dataReceivedFromClient, clientIdReceived);
        } while(!dataReceived);
        
        // send error message to the clients which have not finished data transfer and hang for time more than TIMEOUT_HANG
        if (clientsBeyondTimeout.size() > 0) {
            string errorMsg = "Packages receiving stopped due to timeout\n";
            vector<string> serializedStr;
            serialize(UDP_BUF_LEN, SERVER_ID, errorMsg, serializedStr);
            for (auto it = serializedStr.cbegin(); it != serializedStr.cend(); it++) {
                const string& strToSend = *it;
                // send the result to client
                for (auto itClients = clientsBeyondTimeout.begin(); itClients != clientsBeyondTimeout.end(); itClients++) {
                    int clientId = *itClients;
                    auto itAddress = clientAddress.find(clientId);
                    if (itAddress != clientAddress.end()) {
                        sockaddr_in& address = itAddress->second;
                        socklen_t addrlen = sizeof(address);
#ifdef TEST
                        res = sendto_test(socketDesc, strToSend.c_str(), strToSend.length(), 0, (struct sockaddr*) &address, addrlen);
#else
                        res = sendto(socketDesc, strToSend.c_str(), strToSend.length(), 0, (struct sockaddr*) &address, addrlen);
#endif
                        if (-1 == res)
                        {
                          rc = 6;
                          string what = "sendto failed";
                          throw runtime_error(what);
                        }
                    }
                }
            }
            for (auto itClients = clientsBeyondTimeout.begin(); itClients != clientsBeyondTimeout.end(); itClients++) {
                int clientId = *itClients;
                timeBeginReceiving.erase(timeBeginReceiving.find(clientId));
                clientAddress.erase(clientAddress.find(clientId));
            }
        }
        
        if (dataReceivedFromClient.find(clientIdReceived) != dataReceivedFromClient.end()) {
            ReceivedData& receivedData = dataReceivedFromClient.find(clientIdReceived)->second;
            
            // remove trailing '\0' except the last
            receivedData.receivedStr.erase(remove(receivedData.receivedStr.begin(), receivedData.receivedStr.end(), '\0'), receivedData.receivedStr.end());
            
            // extract numbers from buffer, calculate their sum, write it to buffer
            string sum_str;
            if (blockOk) {
              int calculationRc = sum_string(receivedData.receivedStr, sum_str);
            } else {
              sum_str = "Network problems, cannot calculate sum\n";
            }

            vector<string> serializedSum;
            serialize(UDP_BUF_LEN, SERVER_ID, sum_str, serializedSum);

            for (auto it = serializedSum.cbegin(); it != serializedSum.cend(); it++) {
                const string& strToSend = *it;
                // send the result to client
#ifdef TEST
                res = sendto_test(socketDesc, strToSend.c_str(), strToSend.length(), 0, (struct sockaddr*) &si_client, slen);
#else
                res = sendto(socketDesc, strToSend.c_str(), strToSend.length(), 0, (struct sockaddr*) &si_client, slen);
#endif
                if (-1 == res)
                {
                  rc = 6;
                  string what = "sendto failed";
                  throw runtime_error(what);
                }
            }
        }
            
        auto itEraseData = dataReceivedFromClient.find(clientIdReceived);
        if (itEraseData != dataReceivedFromClient.end()) {
          dataReceivedFromClient.erase(itEraseData);
        }
        auto itEraseTime = timeBeginReceiving.find(clientIdReceived);
        if (itEraseTime != timeBeginReceiving.end()) {
          timeBeginReceiving.erase(itEraseTime);
        }
        auto itEraseAddress = clientAddress.find(clientIdReceived);
        if (itEraseAddress != clientAddress.end()) {
          clientAddress.erase(itEraseAddress);
        }
    }
}