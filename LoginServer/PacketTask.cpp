#include "PacketTask.h"
#include <Windows.h>
#include <cstddef>
#include <format>
#include "DBPoolManager.h"
#include "../Common/PACKET.h"
#include "../Network_Core/TCPConnection.h"
#include "../ODBC_Core/DBStatement.h"
#include "../Utility_Core/JobPool.h"
#include "../Utility_Core/LogManager.h"

CPacketTask::CPacketTask(CJobPool* _jobPool, const CZoneData* _zoneData):
	m_owner(nullptr),
	m_generation(NULL),
	m_jobPool(_jobPool),
	m_zoneData(_zoneData)
{
}

void CPacketTask::Initialize(std::vector<byte>&& _packet, CTCPConnection* _owner, uint64_t _generation)
{
	m_packet = std::move(_packet);
	m_owner = _owner;
	m_generation = _generation;
}

void CPacketTask::Execute()
{
	if (m_owner == nullptr || m_owner->GetGeneration() != m_generation)
	{
		Cleanup();
		return;
	}

	PACKET_HEADER* packetHeader = reinterpret_cast<PACKET_HEADER*>(m_packet.data() + sizeof(PROXY_HEADER));
	if (packetHeader->type == ePacketType::C2S_Login) OnLogin();
	if (packetHeader->type == ePacketType::C2S_Logout) OnLogout();
	if (packetHeader->type == ePacketType::C2S_CreateAccount) OnCreateAccount();
	if (packetHeader->type == ePacketType::C2S_CharacterList) OnCharacterList();
	if (packetHeader->type == ePacketType::C2S_CreateCharacter) OnCreateCharacter();
	if (packetHeader->type == ePacketType::C2S_SelectCharacter) OnSelectCharacter();

	Cleanup();
}

void CPacketTask::OnLogin()
{
	PROXY_HEADER* proxyHeader = reinterpret_cast<PROXY_HEADER*>(m_packet.data());
	PACKET_C2S_LOGIN* recvPacket = reinterpret_cast<PACKET_C2S_LOGIN*>(m_packet.data() + sizeof(PROXY_HEADER));

	std::wstring id(reinterpret_cast<wchar_t*>(recvPacket->text), recvPacket->idSize);
	std::wstring password(
		reinterpret_cast<wchar_t*>(recvPacket->text + (recvPacket->idSize * sizeof(wchar_t))),
		recvPacket->pwSize);

	PACKET_S2C_LOGIN sendPacket;
	sendPacket.proxyHeader = *proxyHeader;
	sendPacket.result = false;

	auto& connectionPool = CDBPoolManager<eDBType::Account>::GetInstance();
	CDBConnection* connection = connectionPool.Acquire();
	if (connection == nullptr)
	{
		if (m_owner->GetGeneration() == m_generation) m_owner->PostSend(MakeShared(sendPacket));
		return;
	}

	uint64_t accountIndex = 0;
	bool result = false;
	{
		CDBStatement statement(connection->GetHandle());
		std::wstring query = std::format(
			L"SELECT accountIndex FROM accounts WHERE id = '{}' AND password = '{}' LIMIT 1",
			EscapeSQL(id), EscapeSQL(password));

		result = statement.ExecuteQuery(query);
		if (result) result = statement.BindColumn(1, SQL_C_UBIGINT, &accountIndex, sizeof(accountIndex), nullptr);
		if (result) result = statement.Fetch();

		if (result)
		{
			SQLLEN rowCount = 0;
			std::wstring updateQuery = std::format(
				L"UPDATE accounts SET loginTime = NOW() WHERE accountIndex = '{}'", accountIndex);
			result = statement.ExecuteNoneQuery(updateQuery, rowCount);
		}
	}

	connectionPool.Release(connection);

	if (result)
	{
		sendPacket.proxyHeader.accountIndex = accountIndex;
		sendPacket.result = true;
		CLogManager::GetInstance().WriteW(eLogLevel::eINFO, L"[Login] Success, ID : %ls", id.c_str());
	}
	else
	{
		CLogManager::GetInstance().WriteW(eLogLevel::eINFO, L"[Login] Fail, ID : %ls", id.c_str());
	}

	if (m_owner->GetGeneration() == m_generation) m_owner->PostSend(MakeShared(sendPacket));
}

void CPacketTask::OnLogout()
{
	PROXY_HEADER* proxyHeader = reinterpret_cast<PROXY_HEADER*>(m_packet.data());

	PACKET_S2C_LOGOUT sendPacket;
	sendPacket.proxyHeader = *proxyHeader;

	auto& connectionPool = CDBPoolManager<eDBType::Account>::GetInstance();
	CDBConnection* connection = connectionPool.Acquire();
	if (connection == nullptr)
	{
		if (m_owner->GetGeneration() == m_generation) m_owner->PostSend(MakeShared(sendPacket));
		return;
	}

	{
		CDBStatement statement(connection->GetHandle());
		SQLLEN rowCount = 0;
		std::wstring query = std::format(
			L"UPDATE accounts SET logoutTime = NOW() WHERE accountIndex = '{}'", proxyHeader->accountIndex);
		sendPacket.result = statement.ExecuteNoneQuery(query, rowCount);
	}

	connectionPool.Release(connection);

	if (m_owner->GetGeneration() == m_generation) m_owner->PostSend(MakeShared(sendPacket));
}

void CPacketTask::OnCreateAccount()
{
	PROXY_HEADER* proxyHeader = reinterpret_cast<PROXY_HEADER*>(m_packet.data());
	PACKET_S2C_CREATE_ACCOUNT sendPacket;
	sendPacket.proxyHeader = *proxyHeader;
	sendPacket.result = false;

	uint64_t packetPrefixSize = offsetof(PACKET_C2S_CREATE_ACCOUNT, text);
	if (m_packet.size() < sizeof(PROXY_HEADER) + packetPrefixSize)
	{
		if (m_owner->GetGeneration() == m_generation) m_owner->PostSend(MakeShared(sendPacket));
		return;
	}

	PACKET_C2S_CREATE_ACCOUNT* recvPacket = reinterpret_cast<PACKET_C2S_CREATE_ACCOUNT*>(
		m_packet.data() + sizeof(PROXY_HEADER));
	uint64_t requiredSize = sizeof(PROXY_HEADER) + packetPrefixSize +
		((static_cast<uint64_t>(recvPacket->idSize) + recvPacket->pwSize) * sizeof(wchar_t));

	if (recvPacket->idSize < 5 || recvPacket->idSize > 15 ||
		recvPacket->pwSize < 4 || recvPacket->pwSize > 15 ||
		m_packet.size() != requiredSize)
	{
		if (m_owner->GetGeneration() == m_generation) m_owner->PostSend(MakeShared(sendPacket));
		return;
	}

	std::wstring id(reinterpret_cast<wchar_t*>(recvPacket->text), recvPacket->idSize);
	std::wstring password(
		reinterpret_cast<wchar_t*>(recvPacket->text + (recvPacket->idSize * sizeof(wchar_t))),
		recvPacket->pwSize);

	auto& connectionPool = CDBPoolManager<eDBType::Account>::GetInstance();
	CDBConnection* connection = connectionPool.Acquire();
	if (connection == nullptr)
	{
		if (m_owner->GetGeneration() == m_generation) m_owner->PostSend(MakeShared(sendPacket));
		return;
	}

	{
		CDBStatement statement(connection->GetHandle());
		SQLLEN rowCount = 0;
		std::wstring query = std::format(
			L"INSERT INTO accounts (id, password) VALUES ('{}', '{}')",
			EscapeSQL(id), EscapeSQL(password));
		sendPacket.result = statement.ExecuteNoneQuery(query, rowCount) && rowCount == 1;
	}

	connectionPool.Release(connection);

	if (sendPacket.result)
	{
		CLogManager::GetInstance().WriteW(eLogLevel::eINFO, L"[CreateAccount] Success, ID : %ls", id.c_str());
	}
	else
	{
		CLogManager::GetInstance().WriteW(eLogLevel::eINFO, L"[CreateAccount] Fail, ID : %ls", id.c_str());
	}

	if (m_owner->GetGeneration() == m_generation) m_owner->PostSend(MakeShared(sendPacket));
}

void CPacketTask::OnCharacterList()
{
	PROXY_HEADER* proxyHeader = reinterpret_cast<PROXY_HEADER*>(m_packet.data());
	if (proxyHeader->accountIndex == 0) return;

	auto& connectionPool = CDBPoolManager<eDBType::Game>::GetInstance();
	CDBConnection* connection = connectionPool.Acquire();
	if (connection == nullptr) return;

	std::vector<CHARACTER> characterList;
	characterList.reserve(2);
	{
		CDBStatement statement(connection->GetHandle());
		std::wstring query = std::format(
			L"SELECT character_index, name, `class`, `Level`, experience, lastZoneId, "
			L"lastPositionX, lastPositionY, lastPositionZ FROM Characters "
			L"WHERE account_index = '{}' ORDER BY character_index LIMIT 2",
			proxyHeader->accountIndex);

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

		bool result = statement.ExecuteQuery(query);
		if (result) result = statement.BindColumn(1, SQL_C_UBIGINT, &characterIndex, sizeof(characterIndex), nullptr);
		if (result) result = statement.BindColumn(2, SQL_C_CHAR, name, sizeof(name), &nameLen);
		if (result) result = statement.BindColumn(3, SQL_C_UBIGINT, &jobClass, sizeof(jobClass), nullptr);
		if (result) result = statement.BindColumn(4, SQL_C_UBIGINT, &level, sizeof(level), nullptr);
		if (result) result = statement.BindColumn(5, SQL_C_UBIGINT, &experience, sizeof(experience), nullptr);
		if (result) result = statement.BindColumn(6, SQL_C_UBIGINT, &lastZoneId, sizeof(lastZoneId), nullptr);
		if (result) result = statement.BindColumn(7, SQL_C_FLOAT, &lastPositionX, sizeof(lastPositionX), nullptr);
		if (result) result = statement.BindColumn(8, SQL_C_FLOAT, &lastPositionY, sizeof(lastPositionY), nullptr);
		if (result) result = statement.BindColumn(9, SQL_C_FLOAT, &lastPositionZ, sizeof(lastPositionZ), nullptr);


		while (result && characterList.size() < 2)
		{
			memset(name, 0, sizeof(name));
			nameLen = 0;

			if (!statement.Fetch()) break;

			CHARACTER character{};
			character.index = characterIndex;
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
			characterList.emplace_back(character);
		}
	}

	connectionPool.Release(connection);

	uint16_t packetSize = static_cast<uint16_t>(sizeof(PACKET_S2C_CHARACTERLIST) + sizeof(CHARACTER) * characterList.size());
	auto sendBuffer = std::make_shared<std::vector<byte>>(packetSize);
	PACKET_S2C_CHARACTERLIST* sendPacket = reinterpret_cast<PACKET_S2C_CHARACTERLIST*>(sendBuffer->data());

	sendPacket->proxyHeader = *proxyHeader;
	sendPacket->packetHeader.size = packetSize - sizeof(PROXY_HEADER);
	sendPacket->packetHeader.type = ePacketType::S2C_CharacterList;
	sendPacket->characterCount = static_cast<byte>(characterList.size());

	if (!characterList.empty())
	{
		memcpy(
			sendBuffer->data() + sizeof(PACKET_S2C_CHARACTERLIST),
			characterList.data(),
			sizeof(CHARACTER) * characterList.size());
	}

	if (m_owner->GetGeneration() == m_generation) m_owner->PostSend(sendBuffer);
}

void CPacketTask::OnCreateCharacter()
{
	PROXY_HEADER* proxyHeader = reinterpret_cast<PROXY_HEADER*>(m_packet.data());
	PACKET_S2C_CREATE_CHARACTER failPacket;
	failPacket.proxyHeader = *proxyHeader;
	if (m_packet.size() < sizeof(PROXY_HEADER) + sizeof(PACKET_C2S_CREATE_CHARACTER))
	{
		if (m_owner->GetGeneration() == m_generation) m_owner->PostSend(MakeShared(failPacket));
		return;
	}

	PACKET_C2S_CREATE_CHARACTER* recvPacket = reinterpret_cast<PACKET_C2S_CREATE_CHARACTER*>(m_packet.data() + sizeof(PROXY_HEADER));

	uint64_t requiredSize = sizeof(PROXY_HEADER) + sizeof(PACKET_C2S_CREATE_CHARACTER) + recvPacket->nameLen;
	if (proxyHeader->accountIndex == 0 ||
		recvPacket->nameLen == 0 || 
		recvPacket->nameLen > 30 ||
		m_packet.size() != requiredSize ||
		(recvPacket->jobClass != eJobClass::Warrior && recvPacket->jobClass != eJobClass::Archer))
	{
		if (m_owner->GetGeneration() == m_generation) m_owner->PostSend(MakeShared(failPacket));
		return;
	}

	const char* name = reinterpret_cast<const char*>(m_packet.data() + sizeof(PROXY_HEADER) + sizeof(PACKET_C2S_CREATE_CHARACTER));
	std::wstring wideName = UTF8ToWide(name, recvPacket->nameLen);
	if (wideName.empty() || wideName.find(L'\0') != std::wstring::npos)
	{
		if (m_owner->GetGeneration() == m_generation) m_owner->PostSend(MakeShared(failPacket));
		return;
	}

	auto& connectionPool = CDBPoolManager<eDBType::Game>::GetInstance();
	CDBConnection* connection = connectionPool.Acquire();
	if (connection == nullptr)
	{
		if (m_owner->GetGeneration() == m_generation) m_owner->PostSend(MakeShared(failPacket));
		return;
	}

	bool result = false;
	uint64_t characterCount = 0;
	uint64_t startZoneId = 0;
	float startPositionX = 0.0f;
	float startPositionY = 0.0f;
	float startPositionZ = 0.0f;
	{
		CDBStatement statement(connection->GetHandle());
		std::wstring query = std::format(
			L"SELECT COUNT(*) FROM Characters WHERE account_index = '{}'",
			proxyHeader->accountIndex);

		result = statement.ExecuteQuery(query);
		if (result) result = statement.BindColumn(1, SQL_C_UBIGINT, &characterCount, sizeof(characterCount), nullptr);
		if (result) result = statement.Fetch();
	}
	if (result)
	{
		ZONE_INFO startZone{};
		result = m_zoneData != nullptr && m_zoneData->FindStartZone(startZone);
		if (result)
		{
			startZoneId = startZone.zoneId;
			startPositionX = startZone.enterPosition.x;
			startPositionY = 0.0f;
			startPositionZ = startZone.enterPosition.z;
		}
	}

	if (result && startZoneId != 0 && characterCount < 2)
	{
		CDBStatement statement(connection->GetHandle());
		SQLLEN rowCount = 0;
		std::wstring query = std::format(
			L"INSERT INTO Characters (account_index, name, `class`, `Level`, experience, "
			L"lastZoneId, lastPositionX, lastPositionY, lastPositionZ) "
			L"VALUES ('{}', '{}', '{}', '1', '0', '{}', '{}', '{}', '{}')",
			proxyHeader->accountIndex,
			EscapeSQL(wideName),
			static_cast<uint64_t>(recvPacket->jobClass),
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

	connectionPool.Release(connection);

	if (!result)
	{
		if (m_owner->GetGeneration() == m_generation) m_owner->PostSend(MakeShared(failPacket));
		return;
	}

	uint16_t packetSize = sizeof(PACKET_S2C_CREATE_CHARACTER) + sizeof(CHARACTER);
	auto sendBuffer = std::make_shared<std::vector<byte>>(packetSize);
	PACKET_S2C_CREATE_CHARACTER* sendPacket =
		reinterpret_cast<PACKET_S2C_CREATE_CHARACTER*>(sendBuffer->data());

	sendPacket->proxyHeader = *proxyHeader;
	sendPacket->packetHeader.size = packetSize - sizeof(PROXY_HEADER);
	sendPacket->packetHeader.type = ePacketType::S2C_CreateCharacter;
	sendPacket->result = true;

	CHARACTER* character = reinterpret_cast<CHARACTER*>(
		sendBuffer->data() + sizeof(PACKET_S2C_CREATE_CHARACTER));
	memset(character, 0, sizeof(CHARACTER));
	character->index = characterIndex;
	character->nameLen = recvPacket->nameLen;
	memcpy(character->name, name, recvPacket->nameLen);
	character->jobClass = recvPacket->jobClass;
	character->level = 1;
	character->experience = 0;
	character->lastZoneId = startZoneId;
	character->lastPositionX = startPositionX;
	character->lastPositionY = startPositionY;
	character->lastPositionZ = startPositionZ;

	if (m_owner->GetGeneration() == m_generation) m_owner->PostSend(sendBuffer);
}

void CPacketTask::OnSelectCharacter()
{
	PROXY_HEADER* proxyHeader = reinterpret_cast<PROXY_HEADER*>(m_packet.data());
	PACKET_S2C_SELECT_CHARACTER sendPacket;
	sendPacket.proxyHeader = *proxyHeader;

	if (proxyHeader->accountIndex == 0 ||
		m_packet.size() != sizeof(PROXY_HEADER) + sizeof(PACKET_C2S_SELECT_CHARACTER))
	{
		if (m_owner->GetGeneration() == m_generation) m_owner->PostSend(MakeShared(sendPacket));
		return;
	}

	PACKET_C2S_SELECT_CHARACTER* recvPacket = reinterpret_cast<PACKET_C2S_SELECT_CHARACTER*>(
		m_packet.data() + sizeof(PROXY_HEADER));
	auto& connectionPool = CDBPoolManager<eDBType::Game>::GetInstance();
	CDBConnection* connection = connectionPool.Acquire();
	if (connection == nullptr)
	{
		if (m_owner->GetGeneration() == m_generation) m_owner->PostSend(MakeShared(sendPacket));
		return;
	}

	uint64_t jobClass = 0;
	bool result = false;
	{
		CDBStatement statement(connection->GetHandle());
		std::wstring query = std::format(
			L"SELECT `class`, `Level`, experience, lastZoneId, lastPositionX, lastPositionY, lastPositionZ "
			L"FROM Characters "
			L"WHERE Characters.character_index = '{}' AND Characters.account_index = '{}' LIMIT 1",
			recvPacket->characterIndex,
			proxyHeader->accountIndex);
		result = statement.ExecuteQuery(query);
		if (result) result = statement.BindColumn(1, SQL_C_UBIGINT, &jobClass, sizeof(jobClass), nullptr);
		if (result) result = statement.BindColumn(2, SQL_C_UBIGINT, &sendPacket.playerData.level, sizeof(uint64_t), nullptr);
		if (result) result = statement.BindColumn(3, SQL_C_UBIGINT, &sendPacket.playerData.experience, sizeof(uint64_t), nullptr);
		if (result) result = statement.BindColumn(4, SQL_C_UBIGINT, &sendPacket.playerData.zoneId, sizeof(uint64_t), nullptr);
		if (result) result = statement.BindColumn(5, SQL_C_FLOAT, &sendPacket.playerData.position.x, sizeof(float), nullptr);
		if (result) result = statement.BindColumn(6, SQL_C_FLOAT, &sendPacket.playerData.position.y, sizeof(float), nullptr);
		if (result) result = statement.BindColumn(7, SQL_C_FLOAT, &sendPacket.playerData.position.z, sizeof(float), nullptr);
		if (result) result = statement.Fetch();
	}

	connectionPool.Release(connection);

	ZONE_INFO zoneInfo{};
	if (result) result = m_zoneData != nullptr && m_zoneData->FindZone(sendPacket.playerData.zoneId, zoneInfo) && zoneInfo.isActive;

	if (result)
	{
		sendPacket.result = eZoneResult::Success;
		sendPacket.playerData.characterIndex = recvPacket->characterIndex;
		sendPacket.playerData.accountIndex = proxyHeader->accountIndex;
		sendPacket.playerData.jobClass = static_cast<eJobClass>(jobClass);
		sendPacket.playerData.maxHP = 100 + (sendPacket.playerData.level - 1) * 20;
		sendPacket.playerData.hp = sendPacket.playerData.maxHP;
		sendPacket.playerData.position.y = 0.0f;
	}

	if (m_owner->GetGeneration() == m_generation) m_owner->PostSend(MakeShared(sendPacket));
}

void CPacketTask::Cleanup()
{
	m_packet.clear();
	m_owner = nullptr;
	m_generation = NULL;

	m_jobPool->Release(this);
}

std::wstring CPacketTask::EscapeSQL(const std::wstring& _text)
{
	std::wstring result;
	result.reserve(_text.size());

	for (uint64_t i = 0; i < _text.size(); ++i)
	{
		if (_text[i] == L'\\') result.push_back(L'\\');
		if (_text[i] == L'\'') result.push_back(L'\'');
		result.push_back(_text[i]);
	}

	return result;
}

std::wstring CPacketTask::UTF8ToWide(const char* _text, int _length)
{
	if (_text == nullptr || _length <= 0) return std::wstring();

	int wideLength = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, _text, _length, nullptr, 0);
	if (wideLength <= 0) return std::wstring();

	std::wstring result(wideLength, L'\0');
	if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, _text, _length, result.data(), wideLength) != wideLength)
	{
		return std::wstring();
	}

	return result;
}
