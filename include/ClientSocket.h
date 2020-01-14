#ifndef  CLIENTSOCKET_INC
#define  CLIENTSOCKET_INC

#include <cstdint>
#include <thread>
#include <mutex>              
#include <condition_variable> 
#include <queue>
#include <unistd.h>
#include <pthread.h>
#include "Types/ServerType.h"
#include "SocketData.h"

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

            /** \brief  Push a packet to write to the socket
             * \param data the data to write
             * \param size the size of the data */
            void pushPacket(std::shared_ptr<uint8_t>& data, uint32_t size);

            /* \brief Add a message to read for this client
             *
             * This function is aimed to be overwrite
             * \param data the data received. Do not keep it, it will be destroyed by the Server at the end of this call
             * \param size the data size*/
            virtual bool feedMessage(uint8_t* data, uint32_t size);

            /** \brief  Close the client */
            void close();

            /** \brief  Is this client still open?
             * \return  true if yes, false otherwise */
            bool isConnected() const {return !m_close;}

            /** \brief  Gets the number of bytes being written to this client
             * \return   the number of bytes to write */
            uint32_t getBytesInWritting() const {return m_bytesInWriting;}

            uint32_t    nbMessage   = 0;    /*!< The number of remaining message*/
            uint32_t    bufferID;           /*!< The buffer ID which this client belongs to (Server information)*/

            SOCKET      socket;             /*!< The Socket associated with this Client*/
            SOCKADDR_IN sockAddr;           /*!< The Socket address information*/
        private:
            std::mutex              m_writeLock;     /*!< The lock of the writing thread*/
            std::condition_variable m_cond;          /*!< The condition variable used for synchronization*/
            std::mutex              m_condMutex;     /*!< The mutex used with the conditional variable*/
            std::thread             m_writeThread;   /*!< The writing thread.*/

            std::queue<SocketData>  m_writeBuffer;   /*!< Queue data to send */
            bool                    m_close = false; /*!< Is the client closed?*/
            uint32_t                m_bytesInWriting = 0; /*!< The number of bytes being written to that client*/
    };
}

#endif
