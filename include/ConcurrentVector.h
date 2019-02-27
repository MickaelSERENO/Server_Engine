#ifndef  CONCURRENTVECTOR_INC
#define  CONCURRENTVECTOR_INC

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <mutex>

namespace sereno
{
    /* \brief ConcurrentData. Store a data and a mutex to lock while this object lives */
    template <typename T>
    class ConcurrentData
    {
        public:
            /* \brief Constructor
             * \param data the data which this object is bound to
             * \param mutex the mutex to unlock at destruction time or via close*/
            ConcurrentData(T* data, std::mutex* mutex) : m_data(data), m_mutex(mutex)
            {
            }

            ConcurrentData(ConcurrentData&& mvt) : m_data(mvt.m_data), m_mutex(mvt.m_mutex)
            {
                mvt.m_data = NULL;
                mvt.m_mutex = NULL;
            }

            /* \brief Destructor
             * Unlock the mutex */
            ~ConcurrentData()
            {
                if(m_mutex)
                    m_mutex->unlock();
            }

            /* \brief Close the object */
            void close()
            {
                m_data = NULL;
                m_mutex->unlock();
            }

            /* \brief Get the pointer value stored in this data
             * \return the pointer data ! */
            T* getPtr()
            {
                return m_data;
            }

            /* \brief Unref this object
             * \return the data bound to this object */
            T& operator*() {return *m_data;};

        private:
            ConcurrentData(const ConcurrentData& copy);
            ConcurrentData& operator=(const ConcurrentData& copy);

            T*          m_data;
            std::mutex* m_mutex;
    };

    /* \brief Reverse Iterator class for the Concurrent Vector */
    template <typename T>
    class ConcurrentVectorReverseIterator : public std::iterator<std::input_iterator_tag, T*, int32_t, T*, T*&>
    {
        public:
            /* \brief Default constructor, position at NULL */
            ConcurrentVectorReverseIterator()
            {}

            /* \brief Constructor. Create this iterator with pos as a starting point 
             * \param pos : the starting point*/
            ConcurrentVectorReverseIterator(T* pos) : m_pos(pos)
            {}

            /* \brief Copy constructor 
             * \param it the iterator to copy*/
            ConcurrentVectorReverseIterator(const ConcurrentVectorReverseIterator& it) : m_pos(it.m_pos)
            {}

            /* \brief Movement constructor 
             * \param it the iterator to move*/
            ConcurrentVectorReverseIterator(ConcurrentVectorReverseIterator&& it) : m_pos(it.m_pos)
            {
                it.m_pos = NULL;
            }

            ConcurrentVectorReverseIterator<T>& operator++()    {--m_pos; return *this;}                              //prefix
            ConcurrentVectorReverseIterator<T>  operator++(int) {return ConcurrentVectorReverseIterator<T>(m_pos-1);} //postfix

            T*&                                operator*()                {return m_pos;}
            T*                                 operator->()               {return m_pos;}
            ConcurrentVectorReverseIterator<T> operator+(int32_t v) const {return m_pos - v;}

            bool operator==(const ConcurrentVectorReverseIterator<T>& rhs) const {return m_pos == rhs.m_pos;}
            bool operator!=(const ConcurrentVectorReverseIterator<T>& rhs) const {return m_pos != rhs.m_pos;}
        private:
            T* m_pos=NULL; /*!< The position of this iterator*/
    };

    /* \brief Iterator class for the Concurrent Vector 
     * Do not use the iterators provided by this Object in a concurrent context : the iterators may be invalidated because of push and some erase / clear can make it unstable. Use instead the operator[] which does the work*/
    template <typename T>
    class ConcurrentVectorIterator : public std::iterator<std::input_iterator_tag, T*, int32_t, T*, T*&>
    {
        public:
            /* \brief Default constructor, position at NULL */
            ConcurrentVectorIterator()
            {}

            /* \brief Constructor. Create this iterator with pos as a starting point 
             * \param pos : the starting point*/
            ConcurrentVectorIterator(T* pos) : m_pos(pos)
            {}

            /* \brief Copy constructor 
             * \param it the iterator to copy*/
            ConcurrentVectorIterator(const ConcurrentVectorIterator& it) : m_pos(it.m_pos)
            {}

            /* \brief Movement constructor 
             * \param it the iterator to move*/
            ConcurrentVectorIterator(ConcurrentVectorIterator&& it) : m_pos(it.m_pos)
            {
                it.m_pos = NULL;
            }

            ConcurrentVectorIterator<T>& operator++()               {++m_pos; return *this;}                    //prefix
            ConcurrentVectorIterator<T>  operator++(int)            {return ConcurrentVectorIterator(m_pos+1);} //postfix
            T*&                          operator*()                {return m_pos;}
            T*                           operator->()               {return m_pos;}
            ConcurrentVectorIterator<T>  operator+(int32_t v) const {return m_pos + v;}

            bool operator==(const ConcurrentVectorIterator<T>& rhs) const {return m_pos == rhs.m_pos;}
            bool operator!=(const ConcurrentVectorIterator<T>& rhs) const {return m_pos != rhs.m_pos;}
        private:
            T* m_pos=NULL; /*!< The position of this iterator*/
    };


    /* \brief A Concurrent Vector thread safe */
    template <typename T, void* Realloc(void*, size_t)=std::realloc>
    class ConcurrentVector
    {
        public:
            typedef ConcurrentVectorIterator<T>              Iterator;
            typedef ConcurrentVectorIterator<const T>        ConstIterator;
            typedef ConcurrentVectorReverseIterator<T>       ReverseIterator;
            typedef ConcurrentVectorReverseIterator<const T> ConstReverseIterator;

            /* \brief The Constructor, initialize the vector with 0 data */
            ConcurrentVector()
            {}

            ConcurrentVector(uint32_t n) : m_allocSize(n)
            {
                m_data = (T*)Realloc(NULL, n*sizeof(T));
            }

            /* \brief The Destructor */
            virtual ~ConcurrentVector()
            {
                if(m_data)
                    free(m_data);
            }

            /* \brief The Operator []. 
             * return a ConcurrentData object which locks this vector while the other object lives (or is not closed)
             * \param i the indice of the data
             * \return ConcurrentData object to use*/
            ConcurrentData<T> operator[](uint32_t i)
            {
                lock();
                return (i < m_size) ? ConcurrentData<T>(m_data+i, &m_lock) : ConcurrentData<T>(NULL, &m_lock);
            }

            /* \brief The Operator []. 
             * return a ConcurrentData object which locks this vector while the other object lives (or is not closed)
             * \param i the indice of the data
             * \return ConcurrentData object to use*/
            ConcurrentData<const T> operator[](uint32_t i) const
            {
                lock();
                return (i < m_size) ? ConcurrentData<const T>(m_data+i, &m_lock) : ConcurrentData<const T>(NULL, &m_lock);
            }

            /* \brief Erase the value at indice "i"
             * \param i the indice at which we have to erase a value 
             * \return true if the indice is valid, false otherwise*/
            bool eraseAt(uint32_t i)
            {
                lock();
                if(i >= m_size)
                {
                    unlock();
                    return false;
                }
                _eraseAt(i);
                unlock();
            }

            /* \brief Erase the data from our array using the == operator
             * \return true on success, false on failure*/
            bool erase(const T& data)
            {
                lock();
                for(uint32_t i = 0; i < m_size; i++)
                {
                    if(m_data[i] == data)
                    {
                        _eraseAt(i);
                        unlock();
                        return true;
                    }
                }

                unlock();
                return false;
            }

            /* \brief Clear the vector. Size=0 at the end of this call */
            void clear()
            {
                lock();
                for(uint32_t i = 0; i < m_size; i++)
                    m_data[i].~T();
                m_size=0;
                unlock();
            }

            /* \brief push back a data into the Vector array.
             * The data will be copied into the array (using the copy constructor)
             * \param data the data to copy*/
            void pushBack(const T& data)
            {
                lock();
                checkRealloc();
                new(m_data+m_size) T(data);
                m_size+=1;
                unlock();
            }

            /* \brief push back a data into the Vector array.
             * The data will be moved into the array (using the movement constructor)
             * \param data the data to move*/
            void pushBack(T&& data)
            {
                emplaceBack(data);
            }

            /* \brief emplaceback, construct a data at the end of the vector
             * \param args the arguments to pass to the constructors */
            template <typename... Args>
            void emplaceBack(Args&&... args)
            {
                lock();
                checkRealloc();
                new(m_data+m_size) T(args...);
                m_size+=1;
                unlock();
            }

            /* \brief Get the current data size
             * \return the current data size (number of value) */
            uint32_t getSize()
            {
                return m_size;
            }

            /* \brief Get the current data capacity
             * \return the current data capacity (number of value) */
            uint32_t getCapacity()
            {
                return m_allocSize;
            }

            /* \brief Tells wheter or not this Vector is empty.
             * However a later push can change this status.
             * \return wheter or not this vector is empty*/
            bool isEmpty()
            {
                lock();
                uint32_t size = m_size;
                unlock();
                return size == 0;
            }


            /* \brief Get the data array
             * \return the data array */
            T* getData()
            {
                return m_data;
            }

            /* \brief Get the begin iterator
             * \return the begin iterator */
            Iterator begin() {return Iterator(m_data);}

            /* \brief Get the end iterator
             * \return the end iterator */
            Iterator end() {return Iterator(m_data+m_size);}

            /* \brief Get the rbegin iterator
             * \return the rbegin iterator */
            ReverseIterator rbegin() {return Iterator(m_data+m_size-1);}

            /* \brief Get the rend iterator
             * \return the rend iterator */
            ReverseIterator rend() {return Iterator(m_data-1);}
        protected:
            /* \brief Lock the Vector. */
            void lock()
            {
                m_lock.lock();
            }

            /* \brief Unlock the Vector. */
            void unlock()
            {
                m_lock.unlock();
            }

            /* \brief Private function erasing without locking or doing checks
             * \param i the indice to erase */
            void _eraseAt(uint32_t i)
            {
                m_data[i].~T();
                for(uint32_t j = i; j < m_size-1; j++)
                    memcpy(m_data+j, m_data+j+1, sizeof(T));
                m_size--;
            }

            /* \brief Check whether or not this object need to allocate more memory */
            void checkRealloc()
            {
                if(m_size == m_allocSize)
                {
                    m_data = (T*)Realloc(m_data, (m_size+16)*sizeof(T));
                    m_allocSize+=16;
                }
            }

            std::mutex m_lock;             /*!< The Mutex used for concurrent thread*/
            uint32_t   m_size      = 0;    /*!< The size of the data currently in use*/
            uint32_t   m_allocSize = 0;    /*!< The size allocated*/
            T*         m_data      = NULL; /*!< The data array*/
    };
}

#endif
