#include "MonsterDBLoader.h"
#include "../ODBC_Core/DBStatement.h"
#include <cmath>

bool CMonsterDBLoader::Initialize(CDBConnectionPool* _connectionPool)
{
	if (_connectionPool == nullptr) return false;
	m_connectionPool = _connectionPool;
	return true;
}

bool CMonsterDBLoader::Load(std::vector<MONSTER_SPAWN_INFO>& _typeList)
{
	_typeList.clear();
	if (m_connectionPool == nullptr) return false;
	CDBConnection* connection = m_connectionPool->Acquire();
	if (connection == nullptr) return false;

	std::vector<MONSTER_SPAWN_INFO> typeList(4);
	bool result = false;
	uint64_t loadedCount = 0;
	{
		CDBStatement statement(connection->GetHandle());
		std::wstring query = L"SELECT monsterType, level, maxHP, experience, attackPower, aggressive, detectionRange, attackRange, moveSpeed, attackIntervalMs, canWander, canChase, canAttack, idleMinTimeMs, idleMaxTimeMs, wanderMinTimeMs, wanderMaxTimeMs, wanderRadius, maxAggroRange, respawnRadius, respawnDelayMs FROM MonsterTypes ORDER BY monsterType";
		uint64_t monsterType = 0;
		uint64_t aggressive = 0;
		uint64_t canWander = 0;
		uint64_t canChase = 0;
		uint64_t canAttack = 0;
		MONSTER_SPAWN_INFO info{};
		result = statement.ExecuteQuery(query);
		if (result) result = statement.BindColumn(1, SQL_C_UBIGINT, &monsterType, sizeof(monsterType), nullptr);
		if (result) result = statement.BindColumn(2, SQL_C_UBIGINT, &info.level, sizeof(info.level), nullptr);
		if (result) result = statement.BindColumn(3, SQL_C_UBIGINT, &info.maxHP, sizeof(info.maxHP), nullptr);
		if (result) result = statement.BindColumn(4, SQL_C_UBIGINT, &info.experience, sizeof(info.experience), nullptr);
		if (result) result = statement.BindColumn(5, SQL_C_UBIGINT, &info.attackPower, sizeof(info.attackPower), nullptr);
		if (result) result = statement.BindColumn(6, SQL_C_UBIGINT, &aggressive, sizeof(aggressive), nullptr);
		if (result) result = statement.BindColumn(7, SQL_C_FLOAT, &info.detectionRange, sizeof(info.detectionRange), nullptr);
		if (result) result = statement.BindColumn(8, SQL_C_FLOAT, &info.attackRange, sizeof(info.attackRange), nullptr);
		if (result) result = statement.BindColumn(9, SQL_C_FLOAT, &info.moveSpeed, sizeof(info.moveSpeed), nullptr);
		if (result) result = statement.BindColumn(10, SQL_C_UBIGINT, &info.attackIntervalMs, sizeof(info.attackIntervalMs), nullptr);
		if (result) result = statement.BindColumn(11, SQL_C_UBIGINT, &canWander, sizeof(canWander), nullptr);
		if (result) result = statement.BindColumn(12, SQL_C_UBIGINT, &canChase, sizeof(canChase), nullptr);
		if (result) result = statement.BindColumn(13, SQL_C_UBIGINT, &canAttack, sizeof(canAttack), nullptr);
		if (result) result = statement.BindColumn(14, SQL_C_UBIGINT, &info.idleMinTimeMs, sizeof(info.idleMinTimeMs), nullptr);
		if (result) result = statement.BindColumn(15, SQL_C_UBIGINT, &info.idleMaxTimeMs, sizeof(info.idleMaxTimeMs), nullptr);
		if (result) result = statement.BindColumn(16, SQL_C_UBIGINT, &info.wanderMinTimeMs, sizeof(info.wanderMinTimeMs), nullptr);
		if (result) result = statement.BindColumn(17, SQL_C_UBIGINT, &info.wanderMaxTimeMs, sizeof(info.wanderMaxTimeMs), nullptr);
		if (result) result = statement.BindColumn(18, SQL_C_FLOAT, &info.wanderRadius, sizeof(info.wanderRadius), nullptr);
		if (result) result = statement.BindColumn(19, SQL_C_FLOAT, &info.maxAggroRange, sizeof(info.maxAggroRange), nullptr);
		if (result) result = statement.BindColumn(20, SQL_C_FLOAT, &info.respawnRadius, sizeof(info.respawnRadius), nullptr);
		if (result) result = statement.BindColumn(21, SQL_C_UBIGINT, &info.respawnDelayMs, sizeof(info.respawnDelayMs), nullptr);

		while (result)
		{
			bool hasRow = false;
			result = statement.Fetch(hasRow);
			if (!result || !hasRow) break;
			if (monsterType >= typeList.size() || typeList[monsterType].level != 0)
			{
				result = false;
				break;
			}
			if (info.level == 0 || info.level > 20 || info.maxHP == 0 || info.attackIntervalMs == 0)
			{
				result = false;
				break;
			}
			if (aggressive > 1 || canWander > 1 || canChase > 1 || canAttack > 1)
			{
				result = false;
				break;
			}
			if (!std::isfinite(info.moveSpeed) || info.moveSpeed <= 0.0f ||
				!std::isfinite(info.wanderRadius) || !std::isfinite(info.maxAggroRange) || !std::isfinite(info.respawnRadius) ||
				info.wanderRadius < 0.0f || info.respawnRadius < 0.0f ||
				info.maxAggroRange < info.wanderRadius || info.maxAggroRange < info.respawnRadius ||
				info.idleMinTimeMs > info.idleMaxTimeMs || info.wanderMinTimeMs > info.wanderMaxTimeMs)
			{
				result = false;
				break;
			}
			info.type = static_cast<eMonsterType>(monsterType);
			info.aggressive = aggressive != 0;
			info.canWander = canWander != 0;
			info.canChase = canChase != 0;
			info.canAttack = canAttack != 0;
			typeList[monsterType] = info;
			++loadedCount;
		}
	}
	// 쿼리 객체 정리가 끝난 다음 연결 반납
	m_connectionPool->Release(connection);
	if (!result || loadedCount == 0) return false;
	_typeList.swap(typeList);
	return true;
}
