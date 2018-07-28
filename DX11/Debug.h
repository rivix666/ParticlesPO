#pragma once
#include <Windows.h>
#include <string>
#include <sstream>

#define LogD( s )                           \
{                                           \
    std::wstringstream ws;                  \
    ws << s;                                \
    OutputDebugString(ws.str().c_str());    \
}

namespace debug
{
    void Out(const wchar_t* format, ...);
}

class CLogProfile
{
public:
    CLogProfile(std::string name);
    ~CLogProfile();

private:
#ifdef _DEBUG
    std::string m_Name;
    CTimer m_Timer;
#endif
};

class CDebugFileLog
{
public:
    CDebugFileLog(std::string fileName, int mode);
    ~CDebugFileLog();

    void WriteDataToFile(float val, std::string str = "");
    void WriteDataToFile(UINT val, std::string str = "");
    void WriteDataToFile(DWORD val, std::string str = "");
    void WriteDataToFile(int val, std::string str = "");

private:
    std::string m_FileName;
};