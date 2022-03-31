#include<string.h> // memset
#include <arpa/inet.h>
#include <unistd.h> // close
#include <iostream>
#include <stdexcept>
#include <vector>
#include <algorithm>

#include <Serialize.h>
#include <Client.h>

using namespace std;

int tcp_client(void)
{
    int socketDesc = 0;
    struct sockaddr_in serverAddress{0};
    string input;
    char buf[TCP_BUF_LEN]{0};
    int res = 0;
    int rc = 0;
    const int clientId = getClientId();

    try {
        socketDesc = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (-1 == socketDesc) {
          rc = 1;
          string what = "Socket not created";
          throw runtime_error(what);
        }

        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port =  htons(PORT_TCP);
        res = inet_aton(SERVER, &serverAddress.sin_addr);
        if (0 == res) 
        {
          rc = 2;
          string what = "Set server address failed";
          throw runtime_error(what);
        }

        res = connect(socketDesc, (struct sockaddr *) &serverAddress, sizeof(serverAddress));
        if (-1 == res) {
          rc = 3;
          string what = "connect failed";
          throw runtime_error(what);
        }

        while(true)
        {
            cout << "Enter numbers or type \'exit\': "<<endl;
            getline(cin, input);
            
            res = input.compare("exit");
            if (0 == res)
            {
              break;
            }

            vector<string> serializedInput;
            serialize(TCP_BUF_LEN, clientId, input, serializedInput);
            
            for (auto it = serializedInput.cbegin(); it != serializedInput.cend(); it++) {
                const string& strToSend = *it;
                res = send(socketDesc, strToSend.c_str(), strToSend.length(), 0);
                if (-1 == res) {
                  rc = 4;
                  string what = "send failed";
                  throw runtime_error(what);
                }
            }
            
            int numPackagesToReceive = 0;
            int numPackagesReceived = 0;
            string receivedStr;
            int blockLen = TCP_BUF_LEN - PACKAGE_HEADER_LEN-1;
            
            do {
            
                //clear the buffer by filling null, it might have previously received data
                memset(&buf[0], 0, TCP_BUF_LEN);

                res = recv(socketDesc, buf, TCP_BUF_LEN,0);
                if (-1 == res) {
                  // error when receive data
                  rc = 5;
                  string what = "recv failed";
                  throw runtime_error(what);
                }
                if (0 == res){
                  //error: server terminated prematurely
                  rc = 6;
                  string what = "server terminated";
                  throw runtime_error(what);
                }
                
                // data received - deserialize it
                string input(buf);
                Package output;
                int deserializeOk = deserialize(TCP_BUF_LEN, input, output);
                if (deserializeOk != 0) {
                  continue;
                }
                if (SERVER_ID != output.clientId) {
                  // this package comes not from the server, skip it
                  continue;
                }
                if (0 == numPackagesToReceive) {
                    // first valid package reveiced - initialize necessary data
                    numPackagesToReceive = output.numBlocks;
                    receivedStr = string(blockLen*numPackagesToReceive+1, 0);
                }
                if (output.numBlocks != numPackagesToReceive) {
                  // skip bad package
                  continue;
                }
                if (output.numBlocks <= 0) {
                  // skip bad package
                  continue;
                }
                if ((output.blockId < 0) || (output.blockId > output.numBlocks))
                {
                  // skip bad package
                  continue;
                }
                // insert data substring into proper position according to blockId
                receivedStr.replace(output.blockId*blockLen, min(blockLen, (int)output.data.length()), output.data);
                numPackagesReceived++;
            
            } while(numPackagesReceived < numPackagesToReceive);
            // remove trailing '\0' except the last
            receivedStr.erase(remove(receivedStr.begin(), receivedStr.end(), '\0'), receivedStr.end());

            cout << "Sum = "<<receivedStr<<endl;
        }
        
        close(socketDesc);
        return 0;
    
    } catch (runtime_error& e) 
    {
        if(socketDesc > 0) {
          close(socketDesc); 
        }
        throw e;
        return rc;
    }
}
