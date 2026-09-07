#include "PlayerStatDB.h"
#include "../ODBC_Core/DBStatement.h"

bool CPlayerStatDB::Initialize(CDBConnectionPool* _connectionPool)
{
	if (_connectionPool == nullptr) return false;
	m_connectionPool = _connectionPool;
	return true;
}

bool CPlayerStatDB::Load()
{
	m_statList.clear();
	m_statList.resize(42);

	if (m_connectionPool == nullptr) return false;
	CDBConnection* connection = m_connectionPool->Acquire();
	if (connection == nullptr) return false;

	uint64_t jobClass = 0;
	PLAYER_LEVEL_STAT stat{};
	bool result = false;
	uint64_t loadedCount = 0;
	{
		CDBStatement statement(connection->GetHandle());
		result = statement.ExecuteQuery(L"SELECT `class`, `level`, requiredExperience, maxHP FROM PlayerLevelStats ORDER BY `class`, `level`");
		if (result) result = statement.BindColumn(1, SQL_C_UBIGINT, &jobClass, sizeof(jobClass), nullptr);
		if (result) result = statement.BindColumn(2, SQL_C_UBIGINT, &stat.level, sizeof(stat.level), nullptr);
		if (result) result = statement.BindColumn(3, SQL_C_UBIGINT, &stat.requiredExperience, sizeof(stat.requiredExperience), nullptr);
		if (result) result = statement.BindColumn(4, SQL_C_UBIGINT, &stat.maxHP, sizeof(stat.maxHP), nullptr);

		while (result)
		{
			bool hasRow = false;
			result = statement.Fetch(hasRow);
			if (!result || !hasRow) break;
			if (jobClass > static_cast<uint64_t>(eClassState::Archer) || stat.level == 0 || stat.level > 20 || stat.maxHP == 0)
			{
				result = false;
				break;
			}

			uint64_t index = jobClass * 21 + stat.level;
			if (m_statList[index].level != 0)
			{
				result = false;
				break;
			}
			stat.jobClass = static_cast<eClassState>(jobClass);
			m_statList[index] = stat;
			++loadedCount;
		}
	}
	// 쿼리 객체 정리가 끝난 다음 연결 반납
	m_connectionPool->Release(connection);
	if (!result || loadedCount != 40)
	{
		m_statList.clear();
		return false;
	}
	return true;
}
