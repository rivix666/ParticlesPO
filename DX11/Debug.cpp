#include "stdafx.h"
#include "Debug.h"
#include <cstdarg>
#include <iostream>
#include <fstream>

// debug
//////////////////////////////////////////////////////////////////////////
void debug::Out(const wchar_t* format, ...)
{
#ifdef _DEBUG
    va_list argptr;
    va_start(argptr, format);
    wchar_t buffer[256];
    vswprintf(buffer, sizeof(buffer) / sizeof(*buffer), format, argptr);
    OutputDebugString(buffer);
    va_end(argptr);
#endif
}

// LogProfile
//////////////////////////////////////////////////////////////////////////
CLogProfile::CLogProfile(std::string name) 
#ifdef _DEBUG
    : m_Name(name)
#endif
{
#ifdef _DEBUG
    m_Timer.setFrequency(TimerType::Seconds);
    m_Timer.startTimer();
#endif
}

CLogProfile::~CLogProfile()
{
#ifdef _DEBUG
    LogD(m_Name.c_str());
    LogD(" ");
    LogD(m_Timer.getElapsedTime());
    LogD("\n");
#endif
}

// DebugFileLog
//////////////////////////////////////////////////////////////////////////
CDebugFileLog::CDebugFileLog(std::string fileName, int mode) : m_FileName(fileName)
{
#ifdef _DEBUG
    std::ofstream myfile;
    myfile.open(m_FileName, mode);
    myfile.close();
#endif
}

CDebugFileLog::~CDebugFileLog()
{
#ifdef _DEBUG
    std::ofstream myfile;
    myfile.open(m_FileName, std::ofstream::app);
    if (myfile.is_open())
    {
        myfile << "\n";
        myfile.close();
    }
#endif
}

void CDebugFileLog::WriteDataToFile(float val, std::string str)
{
#ifdef _DEBUG
    std::ofstream myfile;
    myfile.open(m_FileName, std::ofstream::app);
    if (myfile.is_open())
    {
        myfile << val << str;
        myfile.close();
    }
#endif
}

void CDebugFileLog::WriteDataToFile(UINT val, std::string str)
{
#ifdef _DEBUG
    std::ofstream myfile;
    myfile.open(m_FileName, std::ofstream::app);
    if (myfile.is_open())
    {
        myfile << val << str;
        myfile.close();
    }
#endif
}

void CDebugFileLog::WriteDataToFile(int val, std::string str)
{
#ifdef _DEBUG
    std::ofstream myfile;
    myfile.open(m_FileName, std::ofstream::app);
    if (myfile.is_open())
    {
        myfile << val << str;
        myfile.close();
    }
#endif
}

void CDebugFileLog::WriteDataToFile(DWORD val, std::string str /*= ""*/)
{
#ifdef _DEBUG
    std::ofstream myfile;
    myfile.open(m_FileName, std::ofstream::app);
    if (myfile.is_open())
    {
        myfile << val << str;
        myfile.close();
    }
#endif
}
