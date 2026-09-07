#pragma once
#include <WinSock2.h>
#include <cstdint>
#include <set>
#include <vector>
#include <string>
#include "FieldObject.h"
#include "PlayerState.h"
#include "ClassType.h"
#include "../Common/ZONE.h"

class CPlayerStatDB;

enum class eFieldViewEventType : uint8_t
{
	Enter = 0,
	Leave,
};

struct FIELD_VIEW_EVENT
{
	eFieldViewEventType	type;
	eFieldObjectType	objectType;
	uint64_t			objectNum;
};

class CPlayer : public CFieldObject
{
private:
	uint64_t						m_characterIndex;
	std::string						m_name;
	eClassState						m_class;
	ePlayerState					m_state;
	uint64_t						m_maxHP;
	uint64_t						m_hp;
	uint64_t						m_level;
	uint64_t						m_experience;
	VECTOR3							m_destPosition;
	std::set<CFieldObject*>			m_visibleObjectList;
	std::vector<FIELD_VIEW_EVENT>	m_viewEventList;
	eAttackType						m_attackType;
	uint64_t						m_attackTargetObjectNum;
	uint8_t							m_nextHitIndex;
	const CPlayerStatDB*			m_playerStatDB;

public:
	CPlayer(uint64_t _objectNum, const CPlayerStatDB* _playerStatDB = nullptr);
	~CPlayer() override = default;

	void Initialize(uint64_t _characterIndex, const std::string& _name, eClassState _class, uint64_t _level = 1, uint64_t _experience = 0);
	void Cleanup();

	void UpdatePosition(const VECTOR3& _position);
	void UpdateRotationY(float _rotationY);
	void UpdateDestPosition(const VECTOR3& _dest);
	void UpdateState(ePlayerState _state);
	uint64_t TakeDamage(uint64_t _damage);
	uint64_t RecoverHalfHP();
	bool AddExperience(uint64_t _experience);
	bool StartAttack(eAttackType _attackType, uint64_t _targetObjectNum);
	void ClearAttack();

	void EnterView(CFieldObject* _object);
	void LeaveView(CFieldObject* _object);
	bool IsVisible(CFieldObject* _object);

	eClassState GetClass();
	uint64_t GetCharacterIndex();
	std::string& GetName();
	ePlayerState GetState();
	uint64_t GetMaxHP();
	uint64_t GetHP();
	uint64_t GetLevel();
	uint64_t GetExperience();
	uint64_t GetRequiredExperience();
	eAttackType GetAttackType();
	uint64_t GetAttackTargetObjectNum();
	bool ConsumeRapidHit(uint8_t _hitIndex);
	VECTOR3& GetDestPosition();
	std::set<CFieldObject*>& GetVisibleObjectList();
	std::vector<FIELD_VIEW_EVENT>& GetViewEventList();
	void ClearViewEventList();
};
