#include<string.h> //memset
#include<arpa/inet.h>
#include <unistd.h> // close
#include <iostream>
#include <stdexcept>
#include <vector>
#include <algorithm>

#include <Serialize.h>
#include <Client.h>

using namespace std;

int udp_client(void)
{
    struct sockaddr_in serverAddress{0};
    int socketDesc = 0;
    socklen_t servAddrLen = sizeof(serverAddress);
    char buf[UDP_BUF_LEN]{0};
    string input;
    int res = 0;
    int rc = 0;
    struct timeval timeout;      
    timeout.tv_sec = TIMEOUT_SEC;
    timeout.tv_usec = TIMEOUT_MICROSEC;
    const int clientId = getClientId();
    
    try{

        socketDesc=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (-1 == socketDesc)
        {
          rc = 1;
          string what = "Socket not created";
          throw runtime_error(what);
        }

        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons(PORT_UDP);
        
        res = inet_aton(SERVER , &serverAddress.sin_addr);
        if (0 == res) 
        {
          rc = 2;
          string what = "Set server address failed";
          throw runtime_error(what);
        }
        
        // set timeout
        res = setsockopt (socketDesc, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof timeout);
        if(-1 == res)
        {
          rc = 3;
          string what = "Set receive timeout failed";
          throw runtime_error(what);
        }
        res = setsockopt (socketDesc, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof timeout);
        if(-1 == res)
        {
          rc = 4;
          string what = "Set send timeout failed";
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
            serialize(UDP_BUF_LEN, clientId, input, serializedInput);

            for (auto it = serializedInput.cbegin(); it != serializedInput.cend(); it++) {
                const string& strToSend = *it;
                res = sendto(socketDesc, strToSend.c_str(), strToSend.length() , 0 , (struct sockaddr *) &serverAddress, servAddrLen);
                if (-1 == res)
                {
                  rc = 5;
                  string what = "sendto() failed";
                  throw runtime_error(what);
                }
            }
            
            int numPackagesToReceive = 0;
            int numPackagesReceived = 0;
            string receivedStr;
            int blockLen = UDP_BUF_LEN - PACKAGE_HEADER_LEN-1;
            
            do {

                //clear the buffer by filling null, it might have previously received data
                memset(buf,'\0', UDP_BUF_LEN);
                // Try to receive some data. 
                // If timeout expired or any error happens, close the connection.
                // TODO: what to do if server performs long computations? Two solutions:
                //  - need to send receive timeout to a larger time 
                //  - each time interval a server should send the notification that calculation is not finished yet.
                while(true) {

                  res = recvfrom(socketDesc, buf, UDP_BUF_LEN, 0, (struct sockaddr *) &serverAddress, &servAddrLen);
                  if (-1 == res)
                  {
                      rc = 6;
                      string what = "recvfrom() failed";
                      throw runtime_error(what);
                  }
                  else {
                    // data received, exit from receive loop
                    break;
                  }
              }
              // data received - deserialize it
              string input(buf);
              Package output;
              int deserializeOk = deserialize(UDP_BUF_LEN, input, output);
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
              // insert data substring into proper position according to blockId, because by UDP packages can be received in any order
              receivedStr.replace(output.blockId*blockLen, min(blockLen, (int)output.data.length()), output.data);
              numPackagesReceived++;
            
            } while(numPackagesReceived < numPackagesToReceive);
            // remove trailing '\0' except the last
            receivedStr.erase(remove(receivedStr.begin(), receivedStr.end(), '\0'), receivedStr.end());
            
            cout << "Sum = "<<receivedStr<<endl;
        }
        
        close(socketDesc);
        return 0;
        
    } catch(runtime_error& e) 
    { 
      if(socketDesc > 0) {
        close(socketDesc); 
      }
      throw e;
      return rc; 
    }
}
