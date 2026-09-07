#include "ServerService.h"
#include <iostream>

bool CServerService::Initialize(INI_CONFIG* _config)
{
    if (!_config) return false;
    m_config = _config;

    if (!m_server.Initialize(m_config->iocpThreadSize, m_config->maxClientConnection * 3))
    {
        std::cout << "IOCPServer Init Fail : " << WSAGetLastError() << "\n";
    }

    if (!OnInit()) { return false; }

    return true;
}

bool CServerService::Run()
{
    m_server.Start();

    if (!OnRun()) { return false; }

    return true;
}

void CServerService::Stop()
{
    OnStop();
    m_server.Stop();
}
