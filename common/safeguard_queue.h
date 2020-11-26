//
// Created by wat on 2020/5/22.
//

#ifndef VIDEOCENTER_SAFEGUARD_QUEUE_H
#define VIDEOCENTER_SAFEGUARD_QUEUE_H

#include <map>
#include <deque>
#include <vector>
#include <mutex>
#include <thread>

using namespace std;

namespace BASE{

    template<typename Data>
    class safeguard_queue {
    private:
        std::mutex the_mutex;
        std::deque<Data> the_queue;

        //pthread_cond_t the_qready;
        //pthread_mutex_t the_qlock;

        //std::lock_guard<std::mutex> lock(g_mutex);

    public:

        void push(Data const& data)
        {
            std::lock_guard<std::mutex> lock(the_mutex);
            //pthread_mutex_lock(&the_qlock);
            the_queue.push_back(data);
            //pthread_mutex_unlock(&the_qlock);

            //pthread_cond_signal(&the_qready);
        }

        void push_front(Data const& data)
        {
            std::lock_guard<std::mutex> lock(the_mutex);
            the_queue.push_front(data);
        }

        const bool empty()
        {
            std::lock_guard<std::mutex> lock(the_mutex);
            return the_queue.empty();
        }

        const int get_size()
        {
            std::lock_guard<std::mutex> lock(the_mutex);
            return the_queue.size();
        }

        void clear()
        {
            std::lock_guard<std::mutex> lock(the_mutex);
            the_queue.clear();
        }

        bool pop(Data& popped_value)
        {
            std::lock_guard<std::mutex> lock(the_mutex);
            if (the_queue.empty())
            {
                return false;
            }

            popped_value = the_queue.front();
            the_queue.pop_front();
            return true;
        }

    private:
        void wait_and_pop(Data& popped_value)
        {
            std::lock_guard<std::mutex> lock(the_mutex);
            /*
             while (the_queue.empty())
             {
                 pthread_cond_wait(&the_qready, &the_qlock);
             }
             */

            popped_value = the_queue.front();
            the_queue.pop_front();
        }

    };

    template<typename Data>
    class safeguard_vector {
    private:
        std::mutex the_mutex;
        std::vector<Data> the_vector;

        //pthread_cond_t the_qready;
        //pthread_mutex_t the_qlock;

        //std::lock_guard<std::mutex> lock(g_mutex);

    public:

        void push(Data const& data)
        {
            std::lock_guard<std::mutex> lock(the_mutex);
            //pthread_mutex_lock(&the_qlock);
            the_vector.emplace_back(data);
            //pthread_mutex_unlock(&the_qlock);

            //pthread_cond_signal(&the_qready);
        }

        const bool isEmpty()
        {
            std::lock_guard<std::mutex> lock(the_mutex);
            return the_vector.empty();
        }

        const int get_size()
        {
            std::lock_guard<std::mutex> lock(the_mutex);
            return the_vector.size();
        }

        void clear()
        {
            std::lock_guard<std::mutex> lock(the_mutex);
            the_vector.clear();
        }

        bool pop(Data& popped_value)
        {
            std::lock_guard<std::mutex> lock(the_mutex);
            if (the_vector.empty())
            {
                return false;
            }

            popped_value = the_vector.front();
            the_vector.pop_front();
            return true;
        }

    private:
        void wait_and_pop(Data& popped_value)
        {
            std::lock_guard<std::mutex> lock(the_mutex);
            /*
             while (the_queue.empty())
             {
                 pthread_cond_wait(&the_qready, &the_qlock);
             }
             */

            popped_value = the_vector.front();
            the_vector.pop_front();
        }

    };

    template<typename T1, typename T2>
    class scopeguard_map {
    private:
        std::mutex the_mutex;
        std::map<T1, T2> the_map;

        //pthread_cond_t the_qready;
        //pthread_mutex_t the_qlock;

        //std::lock_guard<std::mutex> lock(g_mutex);

    public:
        void insert(T1 const& t1, T2 const& t2)
        {
            std::lock_guard<std::mutex> lock(the_mutex);
            the_map.insert(make_pair(t1, t2));
        }

        const bool empty()
        {
            std::lock_guard<std::mutex> lock(the_mutex);
            return the_map.empty();
        }

        const int get_size()
        {
            std::lock_guard<std::mutex> lock(the_mutex);
            return the_map.size();
        }

        void clear()
        {
            std::lock_guard<std::mutex> lock(the_mutex);
            the_map.clear();
        }

    };
}

#endif //VIDEOCENTER_SAFEGUARD_QUEUE_H
