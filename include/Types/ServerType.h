#ifndef  SERVERTYPE_INC
#define  SERVERTYPE_INC

#define INVALID_SOCKET -1
#define SOCKET_ERROR   -1
#define closesocket(param) close(param)

#include <sys/socket.h>
#include <arpa/inet.h>

#define SOCK_PATH "/dev/socket/echo_socket"

typedef int                SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr    SOCKADDR;
#endif
