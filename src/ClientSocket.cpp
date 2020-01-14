#include "ClientSocket.h"
#include "utils.h"

namespace sereno
{
    ClientSocket::ClientSocket() : bufferID(0), socket(SOCKET_ERROR)
    {
        m_writeThread = std::thread([this]{
                while(!m_close)
                {
                    bool empty = false;
                    m_writeLock.lock();
                    empty = m_writeBuffer.empty();
                    if(!empty)
                    {
                        std::shared_ptr<uint8_t> data = m_writeBuffer.front().data;
                        int dataSize = m_writeBuffer.front().dataSize;
                        m_writeBuffer.pop();
                        m_bytesInWriting-=dataSize; //We can sure move it, but well...
                        m_writeLock.unlock();
                        write(socket, data.get(), dataSize);
                    }
                        
                    else
                    {
                        m_writeLock.unlock();
                        std::unique_lock<std::mutex> condLock(m_condMutex);
                        m_cond.wait_for(condLock, std::chrono::microseconds(5));
                    }
                }
        });
    }


    ClientSocket::~ClientSocket()
    {
        close();
    }

    bool ClientSocket::feedMessage(uint8_t* data, uint32_t size)
    {
        return true;
    }

    void ClientSocket::close()
    {
        if(m_close)
            return;
        INFO << "Closing this client." << std::endl;
        //Close the writing thread
        m_close = true;

        //Close the socket
        ::close(socket);

        m_cond.notify_one();
        if(m_writeThread.joinable())
            m_writeThread.join();
        INFO << "Finished to close this client." << std::endl;
    }

    void ClientSocket::pushPacket(std::shared_ptr<uint8_t>& data, uint32_t size)
    {
        m_writeLock.lock();
            m_bytesInWriting += size;
            m_writeBuffer.push({data, (int)size});
            m_cond.notify_one();
        m_writeLock.unlock();
    }
}
