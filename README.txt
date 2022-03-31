Simple client-server app.

Author: Victor Vasilyev [victor1vasilyev@yandex.ru]
Description: client sends a string to server. Server finds all numbers in the string and calculates their sum. If no numbers found then original string is returned to client.

How to build: run ./build.bash
How to use: run ./install/Server and ./install/Client, then follow the hints in terminal

How to build tests: run ./build_test.bash
How to use UDP server tests: run ./install/UDP_Server_test [REF_FILE]

REF_FILE is a text file which describes what should be recived from recv() or recv_from() functions and corresponding reference which should be sent by send() or sendto() functions.

Ref file format:
b [text]   block of data received in a text format
w [int]    wait time in seconds (used to imitate timeout)
r [text]   block of data to be sent by server, this is the reference expected from [b] and [w] described above

To support messages longer than TCP or UDP buffer size, the package is split into blocks.
Data block format:
clientId  [int, truncated to 4 digits]  Id who has sent the package, always 1 for server
numBlocks [int, truncated to 4 digits]  Number of data blocks in the package
blockId   [int, truncated to 4 digits]  Block Id in the package
data      [text]                        Message string

Note:
TCP server tests and client tests are not implemented
