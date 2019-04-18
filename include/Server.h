#ifndef  SERVER_INC
#define  SERVER_INC

#include <iostream>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <pthread.h>
#include <cstring>
#include <dlfcn.h>
#include <cstdint>
#include <queue>
#include <map>
#include <thread>
#include <pthread.h>

#include "ClientSocket.h"
#include "Types/ServerType.h"
#include "ConcurrentVector.h"
#include "utils.h"

namespace sereno
{

    /* \brief The Message received by the Server. */
    template <typename T>
    struct SocketMessage
    {
        T client; /*!< The client who sends this message*/
        std::shared_ptr<uint8_t> data;   /*!< The message*/
        uint32_t      size;              /*!< The data size*/

        /* \brief Constructor
         * \param c the client who sends the message
         * \param d the client's data
         * \param s the data size in bytes */
        SocketMessage(T c, std::shared_ptr<uint8_t> d, uint32_t s) : client(c), data(d), size(s)
        {}

        /* \brief Operator= For SocketMessage. Called the copy constructor */
        SocketMessage& operator=(const SocketMessage<T>& copy)
        {
            if(this == &copy)
                return *this;

            new(this) SocketMessage(copy);
            return *this;
        }
    };

    /* \brief The Class Server. It will handles all the communication part with all the potential clients
     * The type T must extends from ClientSocket */
    template <typename T>
    class Server
    {
        public:

            /*----------------------------------------------------------------------------*/
            /*------------------------------PUBLIC FUNCTIONS------------------------------*/
            /*----------------------------------------------------------------------------*/

            /** \brief Server constructor. Initialize the Server
             * \param nbReadThread the number of thread which will handle the received messages 
             * \param port the port to open*/
            Server(uint32_t nbReadThread, uint32_t port) : m_nbReadThread(nbReadThread), m_port(port)
            {
                //Allocate memory for the handle messages thread
                m_buffers       = new std::queue<SocketMessage<T*>>[nbReadThread];
                m_handleThread  = new std::thread*[nbReadThread];
                m_bufferMutexes = new std::mutex[nbReadThread];
            }

            /** \brief the movement constructor
             * \param mvt the object to move*/
            Server(Server&& mvt) : m_clients(std::move(mvt.m_clients)), m_clientTable(std::move(mvt.m_clientTable))
            {
                //Copy data
                m_sock          = mvt.m_sock;
                m_closeThread   = mvt.m_closeThread;
                m_acceptThread  = mvt.m_acceptThread;
                m_readThread    = mvt.m_readThread;
                m_handleThread  = mvt.m_handleThread;
                m_writeThread   = mvt.m_writeThread;
                m_buffers       = mvt.m_buffers;
                m_nbReadThread  = mvt.m_nbReadThread;
                m_currentBuffer = mvt.m_currentBuffer;
                m_port          = mvt.m_port;

                //reset mvt
                mvt.m_sock          = SOCKET_ERROR;
                mvt.m_closeThread   = true;
                mvt.m_acceptThread  = NULL;
                mvt.m_readThread    = NULL;
                mvt.m_handleThread  = NULL;
                mvt.m_writeThread   = NULL;
                mvt.m_buffers       = NULL;
                mvt.m_nbReadThread  = 0;
                mvt.m_currentBuffer = 0;
            }

            /* \brief The destructor. Destroy this object and every attached thread */
            virtual ~Server()
            {
                closeServer();
                if(m_buffers)
                    delete[] m_buffers;
                if(m_handleThread)
                    delete   m_handleThread;
                if(m_bufferMutexes)
                    delete[] m_bufferMutexes;
            }

            /* \brief Launch the Server and all the communication thread associated
             * \return true on success, false otherwise*/
            virtual bool launch()
            {
                m_closeThread = false;
                //Create the socket and make it reusable
                m_sock   = socket(AF_INET, SOCK_STREAM, 0);
                int temp = 1;
                setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, &temp, sizeof(int));

                 //The server address.
                SOCKADDR_IN serverAddr;
                serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
                serverAddr.sin_family      = AF_INET;
                serverAddr.sin_port        = htons(m_port);

                socklen_t serverLength = sizeof(serverAddr);
                int sockErr;

                if(m_sock != SOCKET_ERROR)
                {
                    sockErr = bind(m_sock, (SOCKADDR*)&serverAddr, serverLength);
                    if(sockErr != SOCKET_ERROR)
                    {
                        sockErr = listen(m_sock, 10);
                        if(sockErr != SOCKET_ERROR)
                        {
                            //Launch every thread 
                            m_acceptThread = new std::thread(&Server::acceptConnectionsThread, this);
                            m_readThread   = new std::thread(&Server::readSocketsThread, this);
                            m_writeThread  = new std::thread(&Server::writeSocketThread, this);
                            for(uint32_t i = 0; i < m_nbReadThread; i++)
                                m_handleThread[i] = new std::thread(&Server::handleMessagesThread, this, i);
                        }
                        else
                        {
                            ERROR << "Could not listen the server socket\n";
                            return false;
                        }
                    }
                    else
                    {
                        ERROR << "Could not bind the server socket\n";
                        return false;
                    }
                }
                else
                {
                    ERROR << "Could not create the server socket\n";
                    return false;
                }

                return true;
            }

            /* \brief Wait for the Server to finish */
            void wait()
            {
                if(m_acceptThread)
                {
                    m_acceptThread->join();
                    delete m_acceptThread;
                    m_acceptThread = NULL;
                }

                if(m_readThread)
                {
                    m_readThread->join();
                    delete m_readThread;
                    m_readThread = NULL;
                }

                if(m_handleThread)
                {
                    for(uint32_t i = 0; i < m_nbReadThread; i++)
                    {
                        if(m_handleThread[i])
                        {
                            m_handleThread[i]->join();
                            delete m_handleThread[i];
                            m_handleThread[i] = NULL;
                        }
                    }
                }

                if(m_writeThread)
                {
                    m_writeThread->join();
                    delete m_writeThread;
                    m_writeThread = NULL;
                }

                for(uint32_t i = 0; i < m_nbReadThread; i++)
                {
                    std::thread* t = m_handleThread[i];
                    t->join();
                    delete t;
                    m_handleThread[i] = NULL;
                }
            }

            /* \brief Close the server*/
            virtual void closeServer()
            {
                INFO << "Closing" << std::endl;
                /* Close every Threads */
                m_closeThread = true;
                if(m_acceptThread)
                {
                    if(m_acceptThread->native_handle() != 0)
                    {
                        pthread_cancel(m_acceptThread->native_handle());
                        m_acceptThread->join();
                    }
                    delete m_acceptThread;
                    m_acceptThread = NULL;
                }
                if(m_readThread)
                {
                    if(m_readThread->joinable())
                        m_readThread->join();
                    delete m_readThread;
                    m_readThread = NULL;
                }

                if(m_writeThread)
                {
                    if(m_writeThread->joinable())
                        m_writeThread->join();
                    delete m_writeThread;
                    m_writeThread = NULL;
                }

                if(m_handleThread)
                {
                    for(uint32_t i = 0; i < m_nbReadThread; i++)
                    {
                        if(m_handleThread[i])
                        {
                            pthread_cancel(m_handleThread[i]->native_handle());
                            m_handleThread[i]->join();
                            delete m_handleThread[i];
                            m_handleThread[i] = NULL;
                        }
                    }
                }

                //Close the socket
                //
                //The server
                if(m_sock != SOCKET_ERROR)
                    close(m_sock);
                m_sock = SOCKET_ERROR;

                //The clients
                for(SOCKET* sock : m_clients)
                    close(*sock);

                //Empty data
                m_clients.clear();

                for(auto& client : m_clientTable)
                    delete client.second;
                m_clientTable.clear();

                for(uint32_t i = 0; i < m_nbReadThread; i++)
                {
                    while(!m_buffers[i].empty())
                        m_buffers[i].pop();

                    while(!m_bufferMutexes[i].try_lock())
                        m_bufferMutexes[i].unlock();
                    m_bufferMutexes[i].unlock();
                }

                while(!m_mapMutex.try_lock())
                    m_mapMutex.unlock();
                m_mapMutex.unlock();

                while(!m_writeMutex.try_lock())
                    m_writeMutex.unlock();
                m_writeMutex.unlock();
            }

        protected:
            /* \brief No copy Constructor */
            Server(const Server& copy);

            /* \brief No copy Operator */
            Server& operator=(const Server& copy);

            /*----------------------------------------------------------------------------*/
            /*----------------------------PROTECTED FUNCTIONS-----------------------------*/
            /*----------------------------------------------------------------------------*/

            /* \brief Close a Client socket. You can use m_clientTable to retrieve the ClientSocket object
             * \param client the socket to close. */
            virtual void closeClient(SOCKET client)
            {
                //INFO << "Client Disconnected\n";
                close(client);
                m_mapMutex.lock();
                    T* cs = m_clientTable[client];
                    if(cs != NULL)
                    {
                        cs->isConnected = false;
                        if(cs->nbMessage == 0)
                        {
                            delete cs;
                            m_clientTable.erase(client);
                        }
                    }
                m_mapMutex.unlock();

                m_clients.erase(client);
            }

            /* \brief Thread accepting the incoming connection*/
            void acceptConnectionsThread()
            {
                while(!m_closeThread)
                {
                    //Accept a client (socket)
                    SOCKADDR_IN clientAddr;
                    socklen_t   clientAddrLen = sizeof(clientAddr);
                    SOCKET client = accept(m_sock, (SOCKADDR*)&clientAddr, &clientAddrLen);

                    //No delay
                    int one = 1;
                    setsockopt(client, SOL_TCP, TCP_NODELAY, &one, sizeof(one));
                    //INFO << "New client connected\n";
                    m_mapMutex.lock();
                        //Create a ClientSocket associated
                        T* obj                 = new T();
                        m_clientTable[client]  = obj;
                        obj->bufferID          = m_currentBuffer;
                        obj->socket            = client;
                        obj->sockAddr          = clientAddr;
                        m_clients.pushBack(client);
                    m_mapMutex.unlock();
                    m_currentBuffer        = (m_currentBuffer + 1)%m_nbReadThread;
                }
            }

            void readSocketsThread()
            {
                while(!m_closeThread)
                {
                    std::vector<struct pollfd> readPoll;
                    
                    //Create a pollfd containing all our sockets
                    //The idea is to know every sockets status
                    for(uint32_t i = 0; i < m_clients.getSize(); i++)
                    {
                        auto client = m_clients[i];
                        if(client.getPtr())
                        {
                            struct pollfd pfd = {.fd = *client, .events = POLLIN};
                            readPoll.push_back(pfd);
                        }
                    }
                    poll(readPoll.data(), readPoll.size(), 10);
                    
                    for(auto& pfd : readPoll)
                    {
                        //One error -> disconnection
                        if(pfd.revents & POLLHUP ||
                           pfd.revents & POLLERR)
                        {
                            closeClient(pfd.fd);
                            continue;
                        }

                        //Value to read available
                        if(pfd.revents & POLLIN)
                        {
                            int count;
                            ioctl(pfd.fd, FIONREAD, &count);

                            //No data -> disconnection
                            if(count == 0)
                            {
                                closeClient(pfd.fd);
                                continue;
                            }

                            //Data to read
                            //Push the data to the corresponding buffer
                            else
                            {
                                uint8_t* buf = (uint8_t*)malloc(sizeof(uint8_t)*count);
                                read(pfd.fd, buf, count);
                                m_mapMutex.lock();
                                    T* client = m_clientTable[pfd.fd];
                                    //This case can appears when a message has arrived AFTER that a client has been disconnected.
                                    if(client == NULL) 
                                    {
                                        free(buf);
                                        m_mapMutex.unlock();
                                        continue;
                                    }
                                    client->nbMessage++;
                                m_mapMutex.unlock();

                                m_bufferMutexes[client->bufferID].lock();
                                    std::shared_ptr<uint8_t> sharedBuf(buf, free);
                                    m_buffers[client->bufferID].emplace(client, sharedBuf, count);    
                                m_bufferMutexes[client->bufferID].unlock();
                            }
                        }
                    }
                    std::this_thread::sleep_for(std::chrono::microseconds(10));
                }
            }

            /* \brief Handle the messages received by the clients
             * One buffer has is own handle messages thread
             * \param bufID the buffer for which this thread has been called */
            void handleMessagesThread(uint32_t bufID)
            {
                std::queue<SocketMessage<T*>>& buffer = m_buffers[bufID];
                while(!m_closeThread)
                {
                    //Sleep if no data
                    if(buffer.size() == 0)
                    {
                        usleep(10);
                        continue;
                    }

                    m_bufferMutexes[bufID].lock();
                        SocketMessage<T*>& msg    = buffer.front();
                        T*       client = msg.client;
                        uint32_t size   = msg.size;
                        std::shared_ptr<uint8_t> data = msg.data;
                        buffer.pop();
                    m_bufferMutexes[bufID].unlock();

                    onMessage(bufID, client, data.get(), size);

                    //Delete the client after having parsed every messages
                    m_mapMutex.lock();
                        client->nbMessage--;
                        if(!client->isConnected && client->nbMessage == 0)
                        {
                            m_clientTable.erase(client->socket);
                            delete client;
                        }
                    m_mapMutex.unlock();
                }
            }

            /* \brief Thread handling the write call */
            void writeSocketThread()
            {
                INFO << "In write thread...\n\n";
                while(!m_closeThread)
                {
                    while(true)
                    {
                        m_writeMutex.lock();
                            if(m_writeBuffer.empty())
                            {
                                m_writeMutex.unlock();
                                break;
                            }
                            SocketMessage<int>& msg = m_writeBuffer.front();
                            int      client = msg.client;
                            uint32_t size   = msg.size;
                            std::shared_ptr<uint8_t> data = msg.data;
                            m_writeBuffer.pop();
                        m_writeMutex.unlock();

                        INFO << "Writting " << size << " bytes\n";
                        write(client, data.get(), size);
                    }
                    usleep(10);
                }

                INFO << "Quitting write thread...\n";
            }

            void writeMessage(SocketMessage<int>& msg)
            {
                m_writeMutex.lock();
                    m_writeBuffer.push(msg);
                m_writeMutex.unlock();
            }

            virtual void onMessage(uint32_t bufID, T* client, uint8_t* data, uint32_t size)
            {
                client->feedMessage(data, size);
            }

            /*----------------------------------------------------------------------------*/
            /*----------------------------PROTECTED ATTRIBUTES----------------------------*/
            /*----------------------------------------------------------------------------*/

            SOCKET                         m_sock          = SOCKET_ERROR; /*!< The server socket*/
            ConcurrentVector<SOCKET>       m_clients;                      /*!< The clients*/
            std::map<SOCKET, T*>           m_clientTable;
            bool                           m_closeThread   = true;         /*!< Should we close the threads ?*/
            std::thread*                   m_acceptThread  = NULL;         /*!< The accept connections thread*/
            std::thread*                   m_readThread    = NULL;         /*!< The read sockets thread*/
            std::thread**                  m_handleThread  = NULL;         /*!< The handle messages thread*/
            std::thread*                   m_writeThread   = NULL;         /*!< The write message thread*/
            std::mutex*                    m_bufferMutexes = NULL;         /*!< The buffer mutexes*/
            std::mutex                     m_mapMutex;                     /*!< The map mutex*/
            std::queue<SocketMessage<T*>>* m_buffers;                      /*!< The buffers containing the sockets messages*/
            std::queue<SocketMessage<int>> m_writeBuffer;                  /*!< The write buffer*/
            std::mutex                     m_writeMutex;                   /*!< The mutex associated with the write buffer*/
            uint32_t                       m_nbReadThread;                 /*!< The number of thread which will handles received messages*/
            uint32_t                       m_currentBuffer = 0;            /*!< The current buffer to allocate the next connection*/
            uint32_t                       m_port;                         /*!< The port to open*/
    };
}

#endif
