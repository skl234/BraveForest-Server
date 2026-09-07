#pragma once
#include <memory>
#include "DBEnvironment.h"
#include "DBConnection.h"
#include "DBStatement.h"
#include "DBConnectionPool.h"
#include "../Utility_Core/Singleton.h"

class CODBCDriver : public CSingleton<CODBCDriver>
{
	friend class CSingleton<CODBCDriver>;

private:
	CDBEnvironment m_env;

private:
	CODBCDriver();
	~CODBCDriver();

public:
	bool Initialize();
	std::unique_ptr<CDBConnection> CreateConnection(const std::wstring& _user, const std::wstring& _password, const std::wstring& _dsn);
};
