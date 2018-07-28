#include "stdafx.h"
#include "Timer.h"

bool CTimer::setFrequency(TimerType type)
{
    LARGE_INTEGER freq;
    if (!QueryPerformanceFrequency(&freq))
        return false;

    PCFreq = double(freq.QuadPart) / double(type);
    return true;
}

void CTimer::startTimer()
{
    LARGE_INTEGER start;
    QueryPerformanceCounter(&start);
    CounterStart = start.QuadPart;
}

void CTimer::stopTimer()
{
    LARGE_INTEGER stop;
    QueryPerformanceCounter(&stop);
    CounterStop = stop.QuadPart;
}

double CTimer::getElapsedTime()
{
    stopTimer();
    return ElapsedTime = double(CounterStop - CounterStart) / PCFreq;
}

double CTimer::getLastElapsedTimeWithCalc()
{
    return ElapsedTime = double(CounterStop - CounterStart) / PCFreq;
}

double CTimer::getLastElapsedTime()
{
    return ElapsedTime;
}