#include <string.h> 
#include <unistd.h>   //close 
#include <netinet/in.h> 
#include <vector>
#include <stdexcept>
#include <algorithm>

#include <Math.h>
#include <Server.h>

using namespace std;
     
int tcp_server(void) 
{  
    int reuseSocket = 1;
    int master_socket = 0;
    socklen_t addrlen = 0;
    int new_socket = 0;
    vector<int> client_sockets(MAX_CLIENTS, 0);
    map<int, ReceivedData> dataReceivedFromClient;
    int activity = 0;
    int valread = 0;
    int client_socket = 0;  
    int maxClientSocketId = 0;  
    int res = 0;
    int rc = 0;
    struct sockaddr_in address{0};  
    char buffer[TCP_BUF_LEN]{0};  
    fd_set readfds{0};   //set of socket descriptors 
    struct timeval timeout;
    timeout.tv_sec = TIMEOUT_SEC;
    timeout.tv_usec = TIMEOUT_MICROSEC;
    int blockLen = TCP_BUF_LEN - PACKAGE_HEADER_LEN-1;
    
    try{
        
        for (size_t i = 0 ; i < client_sockets.size() ; i++)  
        {
          ReceivedData emptyStruc;
          dataReceivedFromClient[i] = emptyStruc;
        }
        
        //create a master socket 
        master_socket = socket(AF_INET , SOCK_STREAM , IPPROTO_TCP);
        if (0 == master_socket)
        {  
          rc = 1;
          string what = "Socket not created";
          throw runtime_error(what);
        }  
        
        //set master socket to allow multiple connections  
        res = setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&reuseSocket, sizeof(reuseSocket));
        if (res < 0)  
        {  
          rc = 2;
          string what = "Failed to set socket option";
          throw runtime_error(what);
        }  
        
        //type of socket created 
        address.sin_family = AF_INET;  
        address.sin_port = htons( PORT_TCP );  
        address.sin_addr.s_addr = htonl(INADDR_ANY); 
        addrlen = sizeof(address);
            
        //bind the socket to localhost port  
        res = bind(master_socket, (struct sockaddr *)&address, sizeof(address));
        if (res < 0)  
        {  
          rc = 3;
          string what = "bind failed";
          throw runtime_error(what); 
        }   
            
        //try to specify maximum of 3 pending connections for the master socket 
        res = listen(master_socket, MAX_BACKLOG);
        if (res < 0)  
        {  
          rc = 4;
          string what = "listen failed";
          throw runtime_error(what);
        }  
            
        //accept the incoming connection 
        //map<int, ReceivedData> dataReceivedFromClient;
        while(true)  
        {
            if (needExit()) {
                rc = 0;
                string what = "exit";
                throw runtime_error(what);
            }
          
            //clear the socket set 
            FD_ZERO(&readfds);  
        
            //add master socket to set 
            FD_SET(master_socket, &readfds);  
            maxClientSocketId = master_socket;  
                
            //add child sockets to set 
            for (size_t i = 0 ; i < client_sockets.size() ; i++)  
            {  
                //socket descriptor 
                client_socket = client_sockets[i];  
                    
                //if valid socket descriptor then add to read list 
                if(client_socket > 0) { 
                    FD_SET( client_socket , &readfds);  
                }
                    
                //highest file descriptor number, need it for the select function 
                if(client_socket > maxClientSocketId) { 
                    maxClientSocketId = client_socket; 
                }
            }  
        
            //wait for an activity on one of the sockets 
            //TODO: use poll instead. 
            // Select() has advantage: microseconds timer. But it may not work properly on PC, this is necessary on real time OS.
            // Disadvantages of select(): 
            //   - limited number of connections, 
            //   - need to iterate over all readfds structures to see if data was received on any socket.
            activity = select( maxClientSocketId + 1 , &readfds , NULL , NULL , &timeout);  
          
            if (activity < 0) 
            {  
                rc = 5;
                string what = "select failed";
                throw runtime_error(what);
            }  
                
            //If something happened on the master socket , 
            //then its an incoming connection 
            if (FD_ISSET(master_socket, &readfds))  
            {  
                new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen);
                if (new_socket < 0)  
                {  
                    rc = 6;
                    string what = "accept failed";
                    throw runtime_error(what); 
                }  

                //add new socket to array of sockets 
                for (size_t i = 0; i < client_sockets.size(); i++)  
                {  
                    //if position is empty 
                    if( client_sockets[i] == 0 )  
                    {  
                        client_sockets[i] = new_socket;      
                        break;  
                    }  
                }  
            }  
                
            //else its some IO operation on some other socket
            for (size_t i = 0; i < client_sockets.size(); i++)  
            {  
                client_socket = client_sockets[i];  
                    
                if (FD_ISSET( client_socket , &readfds))  
                {  
                    memset(&buffer[0], 0, TCP_BUF_LEN);

                    //Check if it was for closing , and also read the incoming message 
                    valread = recv( client_socket , buffer, TCP_BUF_LEN, 0);
                    if (valread < 0) {
                        // close the socket which gives error, keep other valid sockets
                        close( client_socket );  
                        client_sockets[i] = 0;
                    }
                    else if (0 == valread)  
                    {  
                        //Somebody disconnected, close the socket and mark as 0 in list for reuse 
                        close( client_socket );  
                        client_sockets[i] = 0;  
                    }  
                    else 
                    {  
                        // data received - deserialize it
                        string input(buffer);
                        Package output;
                        int deserializeOk = deserialize(TCP_BUF_LEN, input, output);
                        if (deserializeOk != 0) {
                          // skip bad package
                          continue;
                        }
                        ReceivedData& receivedData = dataReceivedFromClient.at(i);
                        bool blockOk = true;
                        if (output.numBlocks <= 0) {
                          blockOk = false;
                        }
                        if ((output.blockId < 0) || (output.blockId > output.numBlocks))
                        {
                          blockOk = false;
                        }
                        if (receivedData.numPackagesToReceive != 0)
                        {
                            if (output.numBlocks != receivedData.numPackagesToReceive) {
                              blockOk = false;
                            }
                        }
                        
                        bool dataTransferFinished = false;
                        if (!blockOk)
                        {
                            // TODO: what to do if further data from this client input will be received?
                            // In current implementation the new error messages will be sent to client,
                            // so client can receive many messages from one error.
                            dataTransferFinished = true;
                        } else {
                        
                            if (0 == receivedData.numPackagesToReceive) {
                                // first valid package reveiced - initialize necessary data
                                receivedData.output = output;
                                receivedData.numPackagesToReceive = output.numBlocks;
                                receivedData.receivedStr = string(blockLen*receivedData.numPackagesToReceive+1, 0);
                            }
                            
                            // insert data substring into proper position according to blockId
                            receivedData.receivedStr.replace(output.blockId*blockLen, min(blockLen, (int)output.data.length()), output.data);
                            receivedData.numPackagesReceived++;
                            
                            if (receivedData.numPackagesReceived >= receivedData.numPackagesToReceive) {
                              dataTransferFinished = true;
                              // remove trailing '\0' except the last
                              receivedData.receivedStr.erase(remove(receivedData.receivedStr.begin(), receivedData.receivedStr.end(), '\0'), receivedData.receivedStr.end());
                            }
                        }
 
                        if (dataTransferFinished) {
                            // extract numbers from string, calculate their sum, write it to output
                            string sum_str;
                            if (blockOk) {
                              int calculationRc = sum_string(receivedData.receivedStr, sum_str);
                            } else {
                              sum_str = "Network problems, cannot calculate sum\n";
                            }
                            
                            vector<string> serializedSum;
                            serialize(TCP_BUF_LEN, SERVER_ID, sum_str, serializedSum);

                            for (auto it = serializedSum.cbegin(); it != serializedSum.cend(); it++) {
                                const string& strToSend = *it;
                                res = send(client_socket , strToSend.c_str() , strToSend.length() , 0 );  
                                if (res < 0) {
                                    // close the socket which gives error, keep other valid sockets
                                    close( client_socket );  
                                    client_sockets[i] = 0;
                                }
                            }
                            // nullify map<int, ReceivedData> for this socket, ready to receive new string
                            receivedData.clear();
                        }
                    }  
                }  
            }  
        }
        
        for (size_t i = 0; i < client_sockets.size(); i++)  
        {
            if( client_sockets[i] != 0 )  
            {  
                close( client_sockets[i] );
                client_sockets[i] = 0;
            }
        }
        if (master_socket > 0) {
            close( master_socket );
        }
        setRC_TCP(rc, "");
        return 0;
    
    } catch (runtime_error& e) 
    {
        for (size_t i = 0; i < client_sockets.size(); i++) {
            if( client_sockets[i] != 0 )  
            {  
                close( client_sockets[i] );
                client_sockets[i] = 0;
            }
        }
        if (master_socket > 0) {
            close( master_socket );
        }
        
        setRC_TCP(rc, e.what());
        return rc;
    }
}  
