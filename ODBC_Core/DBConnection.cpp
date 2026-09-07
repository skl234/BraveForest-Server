#include "DBConnection.h"
#include "../Utility_Core/LogManager.h"

CDBConnection::CDBConnection():
	m_hDbc(SQL_NULL_HDBC)
{
}

CDBConnection::~CDBConnection()
{
	Disconnect();
}

bool CDBConnection::Connect(SQLHENV _env, const std::wstring& _user, const std::wstring& _password, const std::wstring& _dsn)
{
    if (m_hDbc)
    {
        CLogManager::GetInstance().Write(eLogLevel::eWARN, "CDBConnection Connect() 중복 시도");
        return true;
    }

    SQLRETURN result;

    result = SQLAllocHandle(SQL_HANDLE_DBC, _env, &m_hDbc);
    if (!SQL_SUCCEEDED(result))
    {
        m_hDbc = SQL_NULL_HDBC;
        CLogManager::GetInstance().Write(eLogLevel::eERROR, "CDBConnection SQLAllocHandle() Error");
        return false;
    }

    result = SQLConnect(m_hDbc,
        const_cast<SQLWCHAR*>(_dsn.c_str()), SQL_NTS,
        const_cast<SQLWCHAR*>(_user.c_str()), SQL_NTS,
        const_cast<SQLWCHAR*>(_password.c_str()), SQL_NTS);

    if (!SQL_SUCCEEDED(result))
    {
        SQLFreeHandle(SQL_HANDLE_DBC, m_hDbc);
        m_hDbc = SQL_NULL_HDBC;
        CLogManager::GetInstance().WriteW(
            eLogLevel::eERROR,
            L"CDBConnection SQLConnect() Error, dsn : %ls , user : %ls",
            _dsn.c_str(), _user.c_str());

        return false;
    }

    return true;
}

void CDBConnection::Disconnect()
{
    if (m_hDbc == SQL_NULL_HDBC) return;

    SQLDisconnect(m_hDbc);
    SQLFreeHandle(SQL_HANDLE_DBC, m_hDbc);
    m_hDbc = SQL_NULL_HDBC;
}

SQLHDBC CDBConnection::GetHandle()
{
    return m_hDbc;
}
