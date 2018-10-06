#include "Server.h"
#include "signal.h"
#include "unistd.h"

Server<ClientSocket> server(5, 8000);

void handleSigactionInt(int num)
{
    server.closeServer();
    exit(0);
}

int main()
{
    //Configuring SIGINT
    struct sigaction intHandler;
    intHandler.sa_handler = handleSigactionInt;
	intHandler.sa_flags   = SA_NOCLDSTOP | SA_NOCLDWAIT;
	sigaction(SIGINT, &intHandler, 0);

    server.launchServer();
    server.waitServer();
    return 0;
}
