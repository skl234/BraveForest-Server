#include "ODBCDriver.h"

CODBCDriver::CODBCDriver()
{
}

CODBCDriver::~CODBCDriver()
{
}

bool CODBCDriver::Initialize()
{
	return m_env.Initialize();
}

std::unique_ptr<CDBConnection> CODBCDriver::CreateConnection(const std::wstring& _user, const std::wstring& _password, const std::wstring& _dsn)
{
	std::unique_ptr<CDBConnection> connection = std::make_unique<CDBConnection>();
	if (!connection->Connect(m_env.GetHandle(), _user, _password, _dsn)) return nullptr;

	return std::move(connection);
}
