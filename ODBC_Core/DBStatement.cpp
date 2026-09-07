#include "DBStatement.h"

CDBStatement::CDBStatement(SQLHDBC _hDbc):
    m_hStmt(SQL_NULL_HSTMT)
{
    SQLRETURN result;
    result = SQLAllocHandle(SQL_HANDLE_STMT, _hDbc, &m_hStmt);
    if (result != SQL_SUCCESS && result != SQL_SUCCESS_WITH_INFO) m_hStmt = SQL_NULL_HSTMT;
}

CDBStatement::~CDBStatement()
{
    if (m_hStmt != SQL_NULL_HSTMT) { SQLFreeHandle(SQL_HANDLE_STMT, m_hStmt); m_hStmt = SQL_NULL_HSTMT; }
}

bool CDBStatement::ExecuteNoneQuery(const std::wstring& _query, SQLLEN& _rowCount)
{
    if (m_hStmt == SQL_NULL_HSTMT) return false;

    _rowCount = 0;

    SQLCloseCursor(m_hStmt);

    SQLRETURN result = SQLExecDirectW(m_hStmt, const_cast<SQLWCHAR*>(_query.c_str()), SQL_NTS);

    if (!SQL_SUCCEEDED(result)) return false;

    result = SQLRowCount(m_hStmt, &_rowCount);
    if (!SQL_SUCCEEDED(result))
    {
        _rowCount = 0;
        return false;
    }

    return true;
}

bool CDBStatement::ExecuteQuery(const std::wstring& _query)
{
    if (m_hStmt == SQL_NULL_HSTMT) return false;

    SQLCloseCursor(m_hStmt);

    SQLRETURN result = SQLExecDirectW(m_hStmt, const_cast<SQLWCHAR*>(_query.c_str()), SQL_NTS);

    if (!SQL_SUCCEEDED(result)) return false;

    return true;
}

bool CDBStatement::BindColumn(SQLUSMALLINT _columnNumber, SQLSMALLINT _targetType,
    SQLPOINTER _targetValue, SQLLEN _bufferLength, SQLLEN* _strLen_or_Ind)
{
    if (m_hStmt == SQL_NULL_HSTMT) return false;

    SQLRETURN result = SQLBindCol(m_hStmt, _columnNumber, _targetType, _targetValue, _bufferLength, _strLen_or_Ind);
    
    if (!SQL_SUCCEEDED(result)) return false;

    return true;
}

bool CDBStatement::Fetch()
{
	bool hasRow = false;
	return Fetch(hasRow) && hasRow;
}

bool CDBStatement::Fetch(bool& _hasRow)
{
	_hasRow = false;
	if (m_hStmt == SQL_NULL_HSTMT) return false;

	SQLRETURN result = SQLFetch(m_hStmt);
	if (result == SQL_NO_DATA) return true;
	if (!SQL_SUCCEEDED(result)) return false;

	_hasRow = true;
	return true;
}
