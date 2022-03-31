#ifndef test_h
#define test_h

#include <sys/types.h>
#include <sys/socket.h>

ssize_t recv_test(int sockfd, void *buf, size_t len, int flags);

ssize_t recvfrom_test(int sockfd, void *buf, size_t len, int flags,
                 struct sockaddr *src_addr, socklen_t *addrlen);

ssize_t send_test(int sockfd, const void *buf, size_t len, int flags);

ssize_t sendto_test(int sockfd, const void *buf, size_t len, int flags,
               const struct sockaddr *dest_addr, socklen_t addrlen);

bool timeout_test(void);

int testFinished(void);

#endif // test_h