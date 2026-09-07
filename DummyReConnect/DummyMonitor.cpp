#include "DummyMonitor.h"
#include <iostream>

void CDummyMonitor::ShowMonitor()
{
    static bool initialized = false;
    static HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    if (!initialized)
    {
        CONSOLE_CURSOR_INFO cursorInfo;
        GetConsoleCursorInfo(hConsole, &cursorInfo);
        cursorInfo.bVisible = FALSE;
        SetConsoleCursorInfo(hConsole, &cursorInfo);

        initialized = true;
    }

    char screen[4096];
    int len = std::snprintf(
        screen,
        sizeof(screen),
        "==============================================================\n"
        "                        DUMMY MONITOR                        \n"
        "==============================================================\n"
        "\n"
        "[갱신단위 : 초]\n"
        " Send Count            : %-10lld\n"
        " Send Fail Count       : %-10lld\n\n"
        " Recv Count            : %-10lld\n"
        "\n"
        "==============================================================\n"
        " Q : Quit\n",
        m_sendCount.load(std::memory_order_relaxed),
        m_sendFailCount.load(std::memory_order_relaxed),
        m_recvCount.load(std::memory_order_relaxed)
    );

    if (len < 0) return;

    COORD pos = { 0,0 };
    SetConsoleCursorPosition(hConsole, pos);

    DWORD written = 0;
    WriteConsoleA(hConsole, screen, static_cast<DWORD>(len), &written, nullptr);
}

void CDummyMonitor::AddSendCount()
{
    m_sendCount.fetch_add(1, std::memory_order_relaxed);
}

void CDummyMonitor::AddSendFailCount()
{
    m_sendFailCount.fetch_add(1, std::memory_order_relaxed);
}

void CDummyMonitor::AddRecvCount()
{
    m_recvCount.fetch_add(1, std::memory_order_relaxed);
}

void CDummyMonitor::Reset()
{
    m_sendCount.store(0, std::memory_order_relaxed);
    m_sendFailCount.store(0, std::memory_order_relaxed);
    m_recvCount.store(0, std::memory_order_relaxed);
}
