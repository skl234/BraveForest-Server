#include "AccountDB.h"
#include "DBPoolManager.h"
#include "../ODBC_Core/DBText.h"
#include "../ODBC_Core/DBStatement.h"
#include <format>

bool CAccountDB::Login(const std::wstring& _id, const std::wstring& _password, uint64_t& _accountIndex)
{
	CDBPoolManager<eDBType::Account>& connectionPool = CDBPoolManager<eDBType::Account>::GetInstance();
	CDBConnection* connection = connectionPool.Acquire();
	if (connection == nullptr) return false;

	_accountIndex = 0;
	bool result = false;
	{
		CDBStatement statement(connection->GetHandle());
		std::wstring query = std::format(
			L"SELECT accountIndex FROM accounts WHERE id = '{}' AND password = '{}' LIMIT 1",
			CDBText::EscapeSQL(_id), CDBText::EscapeSQL(_password));

		result = statement.ExecuteQuery(query);
		if (result) result = statement.BindColumn(1, SQL_C_UBIGINT, &_accountIndex, sizeof(_accountIndex), nullptr);
		if (result) result = statement.Fetch();

		if (result)
		{
			SQLLEN rowCount = 0;
			std::wstring updateQuery = std::format(
				L"UPDATE accounts SET loginTime = NOW() WHERE accountIndex = '{}'", _accountIndex);
			result = statement.ExecuteNoneQuery(updateQuery, rowCount);
		}
	}

	connectionPool.Release(connection);

	return result;
}

bool CAccountDB::Logout(uint64_t _accountIndex)
{
	CDBPoolManager<eDBType::Account>& connectionPool = CDBPoolManager<eDBType::Account>::GetInstance();
	CDBConnection* connection = connectionPool.Acquire();
	if (connection == nullptr) return false;
	bool result = false;

	{
		CDBStatement statement(connection->GetHandle());
		SQLLEN rowCount = 0;
		std::wstring query = std::format(
			L"UPDATE accounts SET logoutTime = NOW() WHERE accountIndex = '{}'", _accountIndex);
		result = statement.ExecuteNoneQuery(query, rowCount);
	}

	connectionPool.Release(connection);

	return result;
}

bool CAccountDB::Create(const std::wstring& _id, const std::wstring& _password)
{
	CDBPoolManager<eDBType::Account>& connectionPool = CDBPoolManager<eDBType::Account>::GetInstance();
	CDBConnection* connection = connectionPool.Acquire();
	if (connection == nullptr) return false;
	bool result = false;

	{
		CDBStatement statement(connection->GetHandle());
		SQLLEN rowCount = 0;
		std::wstring query = std::format(
			L"INSERT INTO accounts (id, password) VALUES ('{}', '{}')",
			CDBText::EscapeSQL(_id), CDBText::EscapeSQL(_password));
		result = statement.ExecuteNoneQuery(query, rowCount) && rowCount == 1;
	}

	connectionPool.Release(connection);

	return result;
}
