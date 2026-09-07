#pragma once
#include <WinSock2.h>
#include <memory>
#include "../Utility_Core/Singleton.h"
#include "../ODBC_Core/DBConnectionPool.h"

enum class eDBType : byte
{
	Account = 0,
	Game,
};

template <eDBType _T>
class CDBPoolManager : public CSingleton<CDBPoolManager<_T>>
{
	friend class CSingleton<CDBPoolManager<_T>>;

private:
	std::unique_ptr<CDBConnectionPool> m_pool;

private:
	CDBPoolManager() = default;
	~CDBPoolManager() = default;

public:
	bool Initialize(const std::wstring& _user, const std::wstring& _password, const std::wstring& _dsn, uint64_t _size)
	{
		m_pool = std::make_unique<CDBConnectionPool>();
		if (m_pool->Initialize(_user, _password, _dsn, _size)) return true;

		m_pool.reset();
		return false;
	}

	CDBConnection* Acquire()
	{
		return m_pool->Acquire();
	}

	void Release(CDBConnection* _connection)
	{
		m_pool->Release(_connection);
	}
};
