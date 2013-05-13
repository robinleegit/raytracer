#ifndef __PROFILER_H__
#define __PROFILER_H__

#include <iostream>
#include <string>
#include <map>

#include "raytracer/CycleTimer.hpp"

namespace _462
{

class Profiler
{
private:
    std::map<std::string, size_t> counts;
    std::map<std::string, double> times;
    std::map<std::string, double> starts;
    std::map<std::string, bool> running;
public:
    ~Profiler()
    {
        Print();
    }
    void Start(std::string key)
    {
        if (running.count(key) == 0)
        {
            times[key] = 0;
            running[key] = true;
        }

        running[key] = true;
        starts[key] = CycleTimer::currentSeconds();
    }
    void Stop(std::string key)
    {
        double stop_time = CycleTimer::currentSeconds();

        if (!running[key])
        {
            // Someone tried to stop a non-running timer
            std::cout << "Throwing because of " << key << std::endl;
            throw;
        }

        running[key] = false;
        times[key] += stop_time - starts[key];
    }
    void Increment(std::string key)
    {
        if (counts.count(key) == 0)
        {
            counts[key] = 1;
        }

        counts[key]++;
    }
    void Print()
    {
        std::cout << std::endl << "Times:" << std::endl;
        std::map<std::string, double>::iterator it;
        for (it = times.begin(); it != times.end(); it++)
        {
            std::cout << it->first << ": " << it->second << "s" << std::endl;
        }

        std::cout << std::endl << "Counts:" << std::endl;
        std::map<std::string, size_t>::iterator it2;
        for (it2 = counts.begin(); it2 != counts.end(); it2++)
        {
            std::cout << it2->first << ": " << it2->second << std::endl;
        }
    }
};

extern Profiler profiler;

}

#define PROFILER
#undef PROFILER

#ifdef PROFILER
#define PROFILER_START(key) profiler.Start(key)
#define PROFILER_STOP(key) profiler.Stop(key)
#else
#define PROFILER_START(key) (key)
#define PROFILER_STOP(key) (key)
#endif

#endif
