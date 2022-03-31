#include<string.h> //memset
#include<arpa/inet.h>
#include <unistd.h> // close
#include <stdexcept>
#include <algorithm>
#include <chrono>
#include <set>

#ifdef TEST
#include <../test/inc.h>
#endif

#include <Serialize.h>
#include <Math.h>
#include <Server.h>

using namespace std;

int udp_server(void)
{
    struct sockaddr_in si_master{0};
    int socketDesc = 0;
    int res = 0;
    int rc = 0;
    struct timeval timeout;      
    timeout.tv_sec = TIMEOUT_SEC;
    timeout.tv_usec = TIMEOUT_MICROSEC;
    
    try {
    
        //create a UDP socket
        socketDesc=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (-1 == socketDesc)
        {
          rc = 1;
          string what = "Socket not created";
          throw runtime_error(what);
        }
        
        // zero out the structure
        memset((char *) &si_master, 0, sizeof(si_master));
        
        si_master.sin_family = AF_INET;
        si_master.sin_port = htons(PORT_UDP);
        si_master.sin_addr.s_addr = htonl(INADDR_ANY);
        
        //bind socket to port
        res = bind(socketDesc , (struct sockaddr*)&si_master, sizeof(si_master) );
        if(-1 == res)
        {
          rc = 2;
          string what = "bind failed";
          throw runtime_error(what);
        }
        
        // set timeout
        res = setsockopt (socketDesc, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof timeout);
        if(-1 == res)
        {
          rc = 3;
          string what = "set receive timeout failed";
          throw runtime_error(what);
        }
        res = setsockopt (socketDesc, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof timeout);
        if(-1 == res)
        {
          rc = 4;
          string what = "set send timeout failed";
          throw runtime_error(what);
        }
        
        //keep listening for data
        UDP_loop(socketDesc);
        
        if (socketDesc > 0) {
            close( socketDesc );
        }
        
        setRC_UDP(rc, "");
        return 0;
    
    } catch (runtime_error& e) 
    {
        if (socketDesc > 0) {
            close( socketDesc );
        }
        
        setRC_UDP(rc, e.what());
        return rc;
    }
}




