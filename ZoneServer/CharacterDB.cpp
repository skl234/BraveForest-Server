#include "CharacterDB.h"
#include "../ODBC_Core/DBStatement.h"

bool CCharacterDB::Initialize(CDBConnectionPool* _connectionPool)
{
	if (_connectionPool == nullptr) return false;
	m_connectionPool = _connectionPool;
	return true;
}

bool CCharacterDB::Load(uint64_t _accountIndex, uint64_t _characterIndex, PLAYER_DATA& _playerData, std::string& _name)
{
	_playerData = {};
	_name.clear();

	if (m_connectionPool == nullptr) return false;
	CDBConnection* connection = m_connectionPool->Acquire();
	if (connection == nullptr) return false;

	uint64_t jobClass = 0;
	char name[31]{};
	SQLLEN nameSize = 0;

	bool result = false;
	{
		CDBStatement statement(connection->GetHandle());
		std::wstring query = L"SELECT `class`, `Level`, experience, lastZoneId, lastPositionX, lastPositionY, lastPositionZ, name FROM Characters WHERE account_index='" +
							 std::to_wstring(_accountIndex) + L"' AND character_index='" + std::to_wstring(_characterIndex) + L"' LIMIT 1";

		result = statement.ExecuteQuery(query);
		if (result) result = statement.BindColumn(1, SQL_C_UBIGINT, &jobClass, sizeof(jobClass), nullptr);
		if (result) result = statement.BindColumn(2, SQL_C_UBIGINT, &_playerData.level, sizeof(uint64_t), nullptr);
		if (result) result = statement.BindColumn(3, SQL_C_UBIGINT, &_playerData.experience, sizeof(uint64_t), nullptr);
		if (result) result = statement.BindColumn(4, SQL_C_UBIGINT, &_playerData.zoneId, sizeof(uint64_t), nullptr);
		if (result) result = statement.BindColumn(5, SQL_C_FLOAT, &_playerData.position.x, sizeof(float), nullptr);
		if (result) result = statement.BindColumn(6, SQL_C_FLOAT, &_playerData.position.y, sizeof(float), nullptr);
		if (result) result = statement.BindColumn(7, SQL_C_FLOAT, &_playerData.position.z, sizeof(float), nullptr);
		if (result) result = statement.BindColumn(8, SQL_C_CHAR, name, sizeof(name), &nameSize);
		if (result) result = statement.Fetch();
	}
	// 쿼리 객체 정리가 끝난 다음 연결 반납
	m_connectionPool->Release(connection);
	if (!result) return false;

	_playerData.accountIndex = _accountIndex;
	_playerData.characterIndex = _characterIndex;
	_playerData.jobClass = static_cast<eJobClass>(jobClass);
	_playerData.maxHP = 0;
	_playerData.hp = 0;
	_playerData.position.y = 0.0f;

	if (nameSize > 0)
	{
		if (nameSize > 30) nameSize = 30;
		_name.assign(name, static_cast<size_t>(nameSize));
	}

	return true;
}

bool CCharacterDB::Save(const PLAYER_DATA& _playerData)
{
	if (m_connectionPool == nullptr) return false;
	CDBConnection* connection = m_connectionPool->Acquire();
	if (connection == nullptr) return false;

	bool result = false;
	{
		CDBStatement statement(connection->GetHandle());
		SQLLEN rowCount = 0;
		std::wstring query = L"UPDATE Characters SET `Level`='" + std::to_wstring(_playerData.level) +
							 L"', experience='" + std::to_wstring(_playerData.experience) + L"', lastZoneId='" + std::to_wstring(_playerData.zoneId) +
							 L"', lastPositionX='" + std::to_wstring(_playerData.position.x) + L"', lastPositionY='0', lastPositionZ='" +
							 std::to_wstring(_playerData.position.z) + L"' WHERE character_index='" + std::to_wstring(_playerData.characterIndex) +
							 L"' AND account_index='" + std::to_wstring(_playerData.accountIndex) + L"'";

		result = statement.ExecuteNoneQuery(query, rowCount);
		if (result && rowCount == 0)
		{
			// 저장값이 그대로면 변경된 행은 0개. 캐릭터가 없는 경우와 구분
			query = L"SELECT character_index FROM Characters WHERE character_index='" + std::to_wstring(_playerData.characterIndex) +
					L"' AND account_index='" + std::to_wstring(_playerData.accountIndex) + L"' LIMIT 1";
			result = statement.ExecuteQuery(query) && statement.Fetch();
		}
	}
	// 쿼리 객체 정리가 끝난 다음 연결 반납
	m_connectionPool->Release(connection);

	return result;
}
