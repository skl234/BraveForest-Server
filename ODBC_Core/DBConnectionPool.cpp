#include "DBConnectionPool.h"
#include "ODBCDriver.h"
#include "../Utility_Core/LockGuard.h"

CDBConnectionPool::CDBConnectionPool()
{
	InitializeSRWLock(&m_lock);
}

CDBConnectionPool::~CDBConnectionPool()
{
}

bool CDBConnectionPool::Initialize(const std::wstring& _user, const std::wstring& _password, const std::wstring& _dsn, uint64_t _size)
{
	if (!m_list.empty()) return false;
	if (_size == 0) return false;
	
	for (uint64_t i = 0; i < _size; ++i)
	{
		std::unique_ptr<CDBConnection> connection = CODBCDriver::GetInstance().CreateConnection(_user, _password, _dsn);
		if (connection == nullptr)
		{
			m_pool.clear();
			m_list.clear();
			return false;
		}

		m_list.emplace_back(std::move(connection));
		m_pool.emplace_back(m_list.back().get());
	}

	return true;
}

CDBConnection* CDBConnectionPool::Acquire()
{
	CLockGuard lock(m_lock);
	if (m_pool.empty()) return nullptr;

	CDBConnection* connection = m_pool.front();
	m_pool.pop_front();

	return connection;
}

void CDBConnectionPool::Release(CDBConnection* _connection)
{
	if (_connection == nullptr) return;

	CLockGuard lock(m_lock);

	m_pool.push_back(_connection);
}
