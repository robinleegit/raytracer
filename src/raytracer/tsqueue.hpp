#ifndef __TSQUEUE_H__
#define __TSQUEUE_H__

#include <queue>
#include <boost/thread.hpp>

template<class T>
class tsqueue
{
private:
    std::queue<T> q;
    boost::mutex mut;
public:
    void Push(T item)
    {
        mut.lock();
        q.push(item);
        mut.unlock();
    }
    T Pop(bool& empty)
    {
        mut.lock();
        empty = q.empty();
        T ret;

        if (empty)
        {
            mut.unlock();
            return ret;
        }

        ret = q.front();
        q.pop();

        mut.unlock();
        return ret;
    }
};

#endif
