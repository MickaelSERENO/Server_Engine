#ifndef  SOCKETDATA_INC
#define  SOCKETDATA_INC

#include <memory>
#include <cstdint>

namespace sereno
{
    struct SocketData
    {
        std::shared_ptr<uint8_t> data;
        int dataSize;
    };
}

#endif
