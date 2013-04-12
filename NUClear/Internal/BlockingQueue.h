/**
 * Copyright (C) 2013 Jake Woods <jake.f.woods@gmail.com>, Trent Houliston <trent@houliston.me>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation 
 * files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, 
 * modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software 
 * is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE 
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR 
 * IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */ 
#ifndef NUCLEAR_BLOCKINGQUEUE_H
#define NUCLEAR_BLOCKINGQUEUE_H

#include <deque>
#include <condition_variable>
#include <mutex>
#include <memory>
#include <exception>
#include <iostream>

namespace NUClear {
namespace Internal {
    
    struct BlockingQueueTerminate : std::exception {
    };
    
    /**
     * T must be movable TODO: Expand
     */
    template <class T>
    class BlockingQueue {
    private:
        std::mutex m_mutex;
        std::condition_variable m_condition;
        std::deque<std::unique_ptr<Reaction>> m_queue;
        
        bool m_stop;
    public:
        BlockingQueue() :
        m_stop(false) {
        }
        
        ~BlockingQueue() {
            std::cerr << "~BlockingQueue" << std::endl;
            m_stop = true;
        }
        
        std::size_t size() {
            return m_queue.size();
        }
        
        void push(T&& data) {
            // We're placing the unique_lock in an anonymous
            // block so that once the data is enqueue'd the lock
            // will be released. This prevents the notified queue
            // trying to access the queue before it is
            // released
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_queue.push_front(std::move(data));
            }
            m_condition.notify_one();
        }
        
        T pop() {
            std::cerr << "BlockingQueue<T>::pop" << std::endl;
            std::unique_lock<std::mutex> lock(this->m_mutex);
            std::cerr << "Acquired lock" << std::endl;
            
            while(!m_stop && m_queue.empty()) {
                std::cerr << "Waiting" << std::endl;
                m_condition.wait(lock);
                std::cerr << "Waiting done" << std::endl;
            }
            
            if(m_stop) {
                std::cerr << "m_stop true, notfying and throwing" << std::endl;
                m_condition.notify_all();
                throw BlockingQueueTerminate();
            }
            
            std::cerr << "getting front (q size:" << m_queue.size() << ")" << std::endl;
            T front = std::move(m_queue.back());
            m_queue.pop_back();
            std::cerr << "got front (q size:" << m_queue.size() << ")" << std::endl;
            
            std::cerr << "BlockingQueue<T>::pop done" << std::endl;
            return std::move(front);
        }
        
        void stop() {
            std::cerr << "BlockingQueue::stop" << std::endl;
            std::unique_lock<std::mutex> lock(m_mutex);
            std::cerr << "Acquired lock" << std::endl;
            m_stop = true;
            std::cerr << "Notifying" << std::endl;
            m_condition.notify_all();
            std::cerr << "Notified" << std::endl;
            std::cerr << "BlockingQueue::stop done" << std::endl;
        }
    };
}
}

#endif
