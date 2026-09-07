#pragma once
#include <WinSock2.h>
#include <sql.h>
#include <sqlext.h>
#include <string>

class CDBStatement
{
private:
	SQLHSTMT m_hStmt;

public:
	CDBStatement(SQLHDBC _hDbc);
	~CDBStatement();

	//Update, Insert, Delete
	bool ExecuteNoneQuery(const std::wstring& _query, SQLLEN& _rowCount);

	//Select
	bool ExecuteQuery(const std::wstring& _query); //noexcept;
	bool BindColumn(SQLUSMALLINT _columnNumber, SQLSMALLINT _targetType,
		SQLPOINTER _targetValue, SQLLEN _bufferLength, SQLLEN* _strLen_or_Ind);
	bool Fetch();
	bool Fetch(bool& _hasRow);
};
