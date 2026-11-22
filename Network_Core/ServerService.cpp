#include "ServerService.h"
#include <cstdlib>
#include <iostream>
#include <new>
#include <system_error>

bool CServerService::Initialize(INI_CONFIG* _config)
{
	const char* section = "CServerService::Initialize";
	try
	{
		if (m_config != nullptr) ExitInitialize(section, ERROR_ALREADY_INITIALIZED);
		if (_config == nullptr) ExitInitialize(section, ERROR_INVALID_PARAMETER);
		if (_config->iocpThreadSize == 0 || _config->iocpThreadSize > UINT32_MAX)
			ExitInitialize(section, ERROR_INVALID_PARAMETER);

		section = "WSAStartup";
		WSADATA data{};
		int error = WSAStartup(MAKEWORD(2, 2), &data);
		if (error != 0) ExitInitialize(section, error);

		section = "CIOCP::Initialize";
		if (!m_iocp.Initialize(_config->iocpConcurrency)) ExitInitialize(section, GetLastError());

		section = "CIOCPEventManager::Initialize";
		if (!m_eventManager.Initialize(_config->recvEventSize, _config->sendEventSize, _config->acceptEventSize))
			ExitInitialize(section, GetLastError());

		section = "CIOCPWorkerManager::Initialize";
		if (!m_workerManager.Initialize(static_cast<uint32_t>(_config->iocpThreadSize)))
			ExitInitialize(section, GetLastError());

		m_config = _config;
		section = "CServerService::OnInit";
		SetLastError(ERROR_SUCCESS);
		if (!OnInit()) ExitInitialize(section, GetLastError());
		return true;
	}
	catch (const std::bad_alloc&)
	{
		ExitInitialize(section, ERROR_NOT_ENOUGH_MEMORY);
	}
	catch (const std::system_error& _error)
	{
		ExitInitialize(section, _error.code().value());
	}
	catch (...)
	{
		ExitInitialize(section, ERROR_UNHANDLED_EXCEPTION);
	}
	return false;
}

bool CServerService::Run()
{
	if (m_config == nullptr || m_workerManager.IsRunning())
		return false;

	StartWorker();

	if (OnRun()) return true;

	Stop();
	return false;
}

bool CServerService::Stop()
{
	if (m_config == nullptr)
		return true;

	if (!m_workerManager.IsRunning() && m_eventManager.GetAcquireSize() != 0)
	{
		StartWorker();
	}

	OnStop();

	while (m_eventManager.GetAcquireSize() != 0)
		Sleep(0);
	if (!m_workerManager.AllJoin(m_iocp))
		return false;
	if (!m_eventManager.Cleanup())
		return false;

	m_iocp.Close();
	WSACleanup();
	m_config = nullptr;

	return true;
}

void CServerService::StartWorker()
{
	const char* section = "CIOCPWorkerManager::StartWorker";
	try
	{
		if (!m_workerManager.StartWorker(m_iocp)) ExitInitialize(section, GetLastError());
	}
	catch (const std::bad_alloc&)
	{
		ExitInitialize(section, ERROR_NOT_ENOUGH_MEMORY);
	}
	catch (const std::system_error& error)
	{
		ExitInitialize(section, error.code().value());
	}
	catch (...)
	{
		ExitInitialize(section, ERROR_UNHANDLED_EXCEPTION);
	}
}

void CServerService::ExitInitialize(const char* _section, const int _error)
{
	int error = _error;
	if (error == ERROR_SUCCESS) error = ERROR_FUNCTION_FAILED;
	std::cout << _section << " error_code=" << error << std::endl;
	std::exit(EXIT_FAILURE);
}
