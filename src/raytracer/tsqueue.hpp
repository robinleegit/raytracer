#pragma once

#include <queue>
#include <mutex>

template<class T>
class tsqueue
{
private:
    std::queue<T> q;
    std::mutex mut;
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
