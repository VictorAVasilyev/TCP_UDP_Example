#include <iostream>
#include <sstream>
#include <stdexcept>
#include <unistd.h>
#include <functional>

#include <Client.h>

using namespace std;

int main(void)
{
    int rc = 0;
    cout << "Client started.\n";
    cout << "Please select protocol: type \'1\' for TCP, \'2\' for UDP\n";
    string input;
    getline(cin, input);
    stringstream ss;
    ss << input;
    int protocol = 0;
    while (!ss.eof()) {
        ss >> protocol;
    }
    if (1 == protocol) {
      try{
        rc = tcp_client();
      } catch (runtime_error& e)
      {
        cout << "Something goes wrong, TCP client returned: "<<e.what()<<endl;
      }
    }
    else if (2 == protocol) {
      try{
        rc = udp_client();
      } catch (runtime_error& e)
      {
        cout << "Something goes wrong, UDP client returned: "<<e.what()<<endl;
      }
    }
    else {
      cout << "Incorrect protocol type\n";
    }
    return 0;
}

int getClientId(void)
{
  //TODO: implement client identification.
  // Current implementation is a simple test solution.
  // It uses has code from hostname.
  // What to do if we run several clients on one host at the same time?
  // One may use something like window handle, or any other identification mechanism
  int clientId = 0;
  char hostname[64]{0};
  int rc = gethostname(hostname, 64);
  if (0 == rc) {
    hash<char*> hashStruc;
    int hashCode = hashStruc(hostname);
    clientId = hashCode % 1000; // truncate to 4 digits as described in package header format
  }
  return clientId;
}
