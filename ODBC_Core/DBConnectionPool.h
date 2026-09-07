#pragma once
#include <WinSock2.h>
#include <memory>
#include <vector>
#include <deque>
#include "DBConnection.h"

class CDBConnectionPool
{
private:
	std::vector<std::unique_ptr<CDBConnection>> m_list;
	std::deque<CDBConnection*>					m_pool;
	SRWLOCK										m_lock;

public:
	CDBConnectionPool();
	~CDBConnectionPool();

	bool Initialize(const std::wstring& _user, const std::wstring& _password, const std::wstring& _dsn, uint64_t _size);

	CDBConnection* Acquire();
	void Release(CDBConnection* _connection);
};
