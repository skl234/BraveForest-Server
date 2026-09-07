#pragma once
#include <WinSock2.h>
#include <sql.h>
#include <sqlext.h>
#include <string>

class CDBConnection
{
private:
	SQLHDBC m_hDbc;

public:
	CDBConnection();
	~CDBConnection();

	bool Connect(SQLHENV _env, const std::wstring& _user, const std::wstring& _password, const std::wstring& _dsn);
	void Disconnect();

	SQLHDBC GetHandle();
};
