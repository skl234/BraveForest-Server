#include "DBEnvironment.h"
#include "../Utility_Core/LogManager.h"

CDBEnvironment::CDBEnvironment():
    m_hEnv(SQL_NULL_HENV)
{
}

CDBEnvironment::~CDBEnvironment()
{
    FreeHandle();
}

bool CDBEnvironment::Initialize()
{
    if (m_hEnv)
    {
        CLogManager::GetInstance().Write(eLogLevel::eWARN, "CDBEnvironment Initialize() 중복 시도");
        return true;
    }

    SQLRETURN result;

    result = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &m_hEnv);
    if (!SQL_SUCCEEDED(result))
    {
        m_hEnv = SQL_NULL_HENV;
        CLogManager::GetInstance().Write(eLogLevel::eERROR, "CDBEnvironment SQLAllocHandle() Error");
        return false;
    }

    result = SQLSetEnvAttr(m_hEnv, SQL_ATTR_ODBC_VERSION, reinterpret_cast<SQLPOINTER>(SQL_OV_ODBC3), SQL_IS_INTEGER);
    if (!SQL_SUCCEEDED(result))
    {
        FreeHandle();
        CLogManager::GetInstance().Write(eLogLevel::eERROR, "CDBEnvironment SQLSetEnvAttr() Error");
        return false;
    }

    return true;
}

void CDBEnvironment::FreeHandle()
{
    if (m_hEnv == SQL_NULL_HENV) return;

    SQLFreeHandle(SQL_HANDLE_ENV, m_hEnv);
    m_hEnv = SQL_NULL_HENV;
}

SQLHENV CDBEnvironment::GetHandle()
{
    return m_hEnv;
}
