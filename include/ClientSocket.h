#ifndef  CLIENTSOCKET_INC
#define  CLIENTSOCKET_INC

#include <cstdint>
#include "Types/ServerType.h"

namespace sereno
{
    /* \brief The ClientSocket class. Handles the client connected to the Server */
    class ClientSocket
    {
        public:
            /* \brief Basic constructor.*/
            ClientSocket();

            /* \brief Basic destructor, does nothing here */
            virtual ~ClientSocket();

            /* \brief Add a message to read for this client
             *
             * This function is aimed to be overwrite
             * \param data the data received. Do not keep it, it will be destroyed by the Server at the end of this call
             * \param size the data size*/
            virtual bool feedMessage(uint8_t* data, uint32_t size);

            uint32_t    nbMessage   = 0;    /*!< The number of remaining message*/
            bool        isConnected = true; /*!< Is the client still connected ?*/
            uint32_t    bufferID;           /*!< The buffer ID which this client belongs to (Server information)*/

            SOCKET      socket;             /*!< The Socket associated with this Client*/
            SOCKADDR_IN sockAddr;           /*!< The Socket address information*/
        private:
    };
}

#endif
