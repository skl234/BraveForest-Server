#include "CharacterDB.h"
#include "../ODBC_Core/DBText.h"
#include "../ODBC_Core/DBStatement.h"
#include <format>

CCharacterDB::CCharacterDB(CDBConnectionPool* _connectionPool) noexcept :
	m_connectionPool(_connectionPool)
{
}

bool CCharacterDB::LoadList(uint64_t _accountIndex, std::vector<CHARACTER>& _characterList) const
{
	_characterList.clear();
	if (m_connectionPool == nullptr || _accountIndex == 0) return false;
	CDBConnection* connection = m_connectionPool->Acquire();
	if (connection == nullptr) return false;

	bool result = false;
	_characterList.reserve(2);
	{
		CDBStatement statement(connection->GetHandle());
		std::wstring query = std::format(
			L"SELECT character_index, name, `class`, `Level`, experience, lastZoneId, "
			L"lastPositionX, lastPositionY, lastPositionZ FROM Characters "
			L"WHERE account_index = '{}' ORDER BY character_index LIMIT 2",
			_accountIndex);

		uint64_t characterIndex = 0;
		char name[31]{};
		SQLLEN nameLen = 0;
		uint64_t jobClass = 0;
		uint64_t level = 0;
		uint64_t experience = 0;
		uint64_t lastZoneId = 0;
		float lastPositionX = 0.0f;
		float lastPositionY = 0.0f;
		float lastPositionZ = 0.0f;

		result = statement.ExecuteQuery(query);
		if (result) result = statement.BindColumn(1, SQL_C_UBIGINT, &characterIndex, sizeof(characterIndex), nullptr);
		if (result) result = statement.BindColumn(2, SQL_C_CHAR, name, sizeof(name), &nameLen);
		if (result) result = statement.BindColumn(3, SQL_C_UBIGINT, &jobClass, sizeof(jobClass), nullptr);
		if (result) result = statement.BindColumn(4, SQL_C_UBIGINT, &level, sizeof(level), nullptr);
		if (result) result = statement.BindColumn(5, SQL_C_UBIGINT, &experience, sizeof(experience), nullptr);
		if (result) result = statement.BindColumn(6, SQL_C_UBIGINT, &lastZoneId, sizeof(lastZoneId), nullptr);
		if (result) result = statement.BindColumn(7, SQL_C_FLOAT, &lastPositionX, sizeof(lastPositionX), nullptr);
		if (result) result = statement.BindColumn(8, SQL_C_FLOAT, &lastPositionY, sizeof(lastPositionY), nullptr);
		if (result) result = statement.BindColumn(9, SQL_C_FLOAT, &lastPositionZ, sizeof(lastPositionZ), nullptr);


		while (result && _characterList.size() < 2)
		{
			memset(name, 0, sizeof(name));
			nameLen = 0;

			if (!statement.Fetch()) break;

			CHARACTER character{};
			character.index = characterIndex;
			if (nameLen <= 0 || nameLen > 30) continue;
			character.nameLen = nameLen;
			if (character.nameLen > sizeof(character.name)) character.nameLen = sizeof(character.name);
			memcpy(character.name, name, character.nameLen);
			character.jobClass = static_cast<eJobClass>(jobClass);
			character.level = level;
			character.experience = experience;
			character.lastZoneId = lastZoneId;
			character.lastPositionX = lastPositionX;
			character.lastPositionY = lastPositionY;
			character.lastPositionZ = lastPositionZ;
			_characterList.emplace_back(character);
		}
	}

	m_connectionPool->Release(connection);

	return result;
}

bool CCharacterDB::Create(uint64_t _accountIndex, eJobClass _class, const std::string& _name, const ZONE_INFO& _startZone, CHARACTER& _character) const
{
	_character = {};
	if (m_connectionPool == nullptr || _accountIndex == 0 || _name.empty() || _name.size() > 30) return false;
	if (_class != eJobClass::Warrior && _class != eJobClass::Archer) return false;
	if (_startZone.zoneId == 0 || !_startZone.isActive || !_startZone.isStartZone) return false;
	std::wstring wideName = CDBText::UTF8ToWide(_name.data(), static_cast<int>(_name.size()));
	if (wideName.empty() || wideName.find(L'\0') != std::wstring::npos || _name.size() > 30) return false;
	CDBConnection* connection = m_connectionPool->Acquire();
	if (connection == nullptr) return false;

	bool result = false;
	uint64_t characterCount = 0;
	uint64_t startZoneId = _startZone.zoneId;
	float startPositionX = _startZone.enterPosition.x;
	float startPositionY = 0.0f;
	float startPositionZ = _startZone.enterPosition.z;
	{
		CDBStatement statement(connection->GetHandle());
		std::wstring query = std::format(
			L"SELECT COUNT(*) FROM Characters WHERE account_index = '{}'",
			_accountIndex);

		result = statement.ExecuteQuery(query);
		if (result) result = statement.BindColumn(1, SQL_C_UBIGINT, &characterCount, sizeof(characterCount), nullptr);
		if (result) result = statement.Fetch();
	}

	if (result && startZoneId != 0 && characterCount < 2)
	{
		CDBStatement statement(connection->GetHandle());
		SQLLEN rowCount = 0;
		std::wstring query = std::format(
			L"INSERT INTO Characters (account_index, name, `class`, `Level`, experience, "
			L"lastZoneId, lastPositionX, lastPositionY, lastPositionZ) "
			L"VALUES ('{}', '{}', '{}', '1', '0', '{}', '{}', '{}', '{}')",
			_accountIndex,
			CDBText::EscapeSQL(wideName),
			static_cast<uint64_t>(_class),
			startZoneId,
			startPositionX,
			startPositionY,
			startPositionZ);

		result = statement.ExecuteNoneQuery(query, rowCount) && rowCount == 1;
	}
	else
	{
		result = false;
	}

	uint64_t characterIndex = 0;
	if (result)
	{
		CDBStatement statement(connection->GetHandle());
		result = statement.ExecuteQuery(L"SELECT LAST_INSERT_ID()");
		if (result) result = statement.BindColumn(1, SQL_C_UBIGINT, &characterIndex, sizeof(characterIndex), nullptr);
		if (result) result = statement.Fetch();
	}

	m_connectionPool->Release(connection);

	if (!result) return false;
	_character = {};
	_character.index = characterIndex;
	_character.nameLen = static_cast<uint8_t>(_name.size());
	memcpy(_character.name, _name.data(), static_cast<uint8_t>(_name.size()));
	_character.jobClass = _class;
	_character.level = 1;
	_character.experience = 0;
	_character.lastZoneId = startZoneId;
	_character.lastPositionX = startPositionX;
	_character.lastPositionY = startPositionY;
	_character.lastPositionZ = startPositionZ;

	return true;
}

bool CCharacterDB::Load(uint64_t _accountIndex, uint64_t _characterIndex, PLAYER_DATA& _data, std::string& _name) const
{
	_data = {};
	_name.clear();
	if (m_connectionPool == nullptr || _accountIndex == 0 || _characterIndex == 0) return false;
	CDBConnection* connection = m_connectionPool->Acquire();
	if (connection == nullptr) return false;

	uint64_t jobClass = 0;
	char name[31]{};
	SQLLEN nameSize = 0;
	bool result = false;
	{
		CDBStatement statement(connection->GetHandle());
		std::wstring query = std::format(
			L"SELECT `class`, `Level`, experience, lastZoneId, lastPositionX, lastPositionY, lastPositionZ, name "
			L"FROM Characters "
			L"WHERE Characters.character_index = '{}' AND Characters.account_index = '{}' LIMIT 1",
			_characterIndex,
			_accountIndex);
		result = statement.ExecuteQuery(query);
		if (result) result = statement.BindColumn(1, SQL_C_UBIGINT, &jobClass, sizeof(jobClass), nullptr);
		if (result) result = statement.BindColumn(2, SQL_C_UBIGINT, &_data.level, sizeof(uint64_t), nullptr);
		if (result) result = statement.BindColumn(3, SQL_C_UBIGINT, &_data.experience, sizeof(uint64_t), nullptr);
		if (result) result = statement.BindColumn(4, SQL_C_UBIGINT, &_data.zoneId, sizeof(uint64_t), nullptr);
		if (result) result = statement.BindColumn(5, SQL_C_FLOAT, &_data.position.x, sizeof(float), nullptr);
		if (result) result = statement.BindColumn(6, SQL_C_FLOAT, &_data.position.y, sizeof(float), nullptr);
		if (result) result = statement.BindColumn(7, SQL_C_FLOAT, &_data.position.z, sizeof(float), nullptr);
		if (result) result = statement.BindColumn(8, SQL_C_CHAR, name, sizeof(name), &nameSize);
		if (result) result = statement.Fetch();
	}

	m_connectionPool->Release(connection);

	if (!result || nameSize <= 0 || nameSize > 30) return false;
	_data.characterIndex = _characterIndex;
	_data.accountIndex = _accountIndex;
	_data.jobClass = static_cast<eJobClass>(jobClass);
	_data.position.y = 0.0f;
	_name.assign(name, static_cast<size_t>(nameSize));
	return true;
}
