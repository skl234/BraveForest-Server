#pragma once
#include <WinSock2.h>
#include <sql.h>
#include <sqlext.h>

class CDBEnvironment
{
private:
	SQLHENV m_hEnv;

public:
	CDBEnvironment();
	~CDBEnvironment();

	bool Initialize();
	void FreeHandle();

	SQLHENV GetHandle();
};
