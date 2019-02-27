#include "ClientSocket.h"

namespace sereno
{
    ClientSocket::ClientSocket() : bufferID(0), socket(SOCKET_ERROR)
    {}

    ClientSocket::~ClientSocket()
    {}

    bool ClientSocket::feedMessage(uint8_t* data, uint32_t size)
    {
        return true;
    }
}
