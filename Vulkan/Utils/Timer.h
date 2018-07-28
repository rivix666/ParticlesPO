#pragma once

enum class ETimerType
{
    Seconds         = 1,
    MiliSeconds     = 1000,
    MicroSeconds    = 1000000
};

class CTimer
{
public:
    CTimer() = default;
    CTimer(ETimerType type);
    ~CTimer() = default;

    bool setFrequency(ETimerType type);
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