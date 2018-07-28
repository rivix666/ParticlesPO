#pragma once

enum TimerType
{
    Seconds         = 1,
    MiliSeconds     = 1000,
    MicroSeconds    = 1000000
};

class CTimer
{
public:
    bool setFrequency(TimerType type);
    void startTimer();
    void stopTimer();
    double getElapsedTime();
    double getLastElapsedTimeWithCalc();
    double getLastElapsedTime();

private:
    double PCFreq;
    __int64 CounterStart;
    __int64 CounterStop;
    double ElapsedTime;
};

