#include "Player.h"
#include "PlayerStatDB.h"

CPlayer::CPlayer(uint64_t _objectNum, const CPlayerStatDB* _playerStatDB) :
	CFieldObject(eFieldObjectType::Player, _objectNum),
	m_characterIndex(0),
	m_class(eClassState::Warrior),
	m_state(ePlayerState::Idle),
	m_maxHP(100),
	m_hp(100),
	m_level(1),
	m_experience(0),
	m_destPosition({0.0f, 0.0f, 0.0f}),
	m_attackType(eAttackType::None),
	m_attackTargetObjectNum(UINT64_MAX),
	m_nextHitIndex(0),
	m_playerStatDB(_playerStatDB)
{
}

void CPlayer::Initialize(uint64_t _characterIndex, const std::string& _name, eClassState _class, uint64_t _level, uint64_t _experience)
{
	m_characterIndex = _characterIndex;
	m_name = _name;
	m_class = _class;
	m_state = ePlayerState::Idle;
	m_level = _level;
	if (m_level == 0) m_level = 1;
	m_experience = _experience;
	const PLAYER_LEVEL_STAT* stat = nullptr;
	if (m_playerStatDB != nullptr) stat = m_playerStatDB->Find(m_class, m_level);
	if (stat != nullptr)
		m_maxHP = stat->maxHP;
	else
		m_maxHP = 100 + (m_level - 1) * 20;
	m_hp = m_maxHP;
	m_destPosition = {0.0f, 0.0f, 0.0f};
	m_visibleObjectList.clear();
	m_viewEventList.clear();
	m_attackType = eAttackType::None;
	m_attackTargetObjectNum = UINT64_MAX;
	m_nextHitIndex = 0;
}

void CPlayer::Cleanup()
{
	m_characterIndex = 0;
	m_name.clear();
	m_class = eClassState::Warrior;
	m_state = ePlayerState::Idle;
	m_maxHP = 100;
	m_hp = 100;
	m_level = 1;
	m_experience = 0;
	m_destPosition = {0.0f, 0.0f, 0.0f};
	m_position = {0.0f, 0.0f, 0.0f};
	m_rotationY = 0.0f;
	m_visibleObjectList.clear();
	m_viewEventList.clear();
	m_attackType = eAttackType::None;
	m_attackTargetObjectNum = UINT64_MAX;
	m_nextHitIndex = 0;
}

void CPlayer::UpdatePosition(const VECTOR3& _position)
{
	m_position = _position;
	m_position.y = 0.0f;
}

void CPlayer::UpdateRotationY(float _rotationY)
{
	m_rotationY = _rotationY;
}

void CPlayer::UpdateDestPosition(const VECTOR3& _dest)
{
	m_destPosition = _dest;
	m_destPosition.y = 0.0f;
}

void CPlayer::UpdateState(ePlayerState _state)
{
	if (m_state == ePlayerState::Die) return;
	m_state = _state;
}

uint64_t CPlayer::TakeDamage(uint64_t _damage)
{
	if (m_state == ePlayerState::Die) return 0;
	uint64_t appliedDamage = _damage;
	if (appliedDamage > m_hp) appliedDamage = m_hp;
	m_hp -= appliedDamage;
	if (m_hp == 0)
	{
		m_state = ePlayerState::Die;
		m_destPosition = m_position;
		ClearAttack();
	}
	return appliedDamage;
}

uint64_t CPlayer::RecoverHalfHP()
{
	if (m_state == ePlayerState::Die || m_hp >= m_maxHP) return 0;
	uint64_t recoveryHP = m_maxHP / 2;
	if (recoveryHP > m_maxHP - m_hp) recoveryHP = m_maxHP - m_hp;
	m_hp += recoveryHP;
	return recoveryHP;
}

bool CPlayer::AddExperience(uint64_t _experience)
{
	if (m_state == ePlayerState::Die || m_level >= 20) return false;
	bool levelUp = false;
	m_experience += _experience;
	while (m_level < 20)
	{
		uint64_t requiredExperience = GetRequiredExperience();
		if (requiredExperience == 0) break;
		if (m_experience < requiredExperience) break;
		m_experience -= requiredExperience;
		++m_level;
		const PLAYER_LEVEL_STAT* stat = nullptr;
		if (m_playerStatDB != nullptr) stat = m_playerStatDB->Find(m_class, m_level);
		if (stat != nullptr)
			m_maxHP = stat->maxHP;
		else
			m_maxHP = 100 + (m_level - 1) * 20;
		m_hp = m_maxHP;
		levelUp = true;
	}
	return levelUp;
}

bool CPlayer::StartAttack(eAttackType _attackType, uint64_t _targetObjectNum)
{
	if (m_state == ePlayerState::Die || _attackType == eAttackType::None) return false;
	if (m_attackType != eAttackType::None) return false;
	m_attackType = _attackType;
	m_attackTargetObjectNum = _targetObjectNum;
	m_nextHitIndex = 0;
	m_state = ePlayerState::Attack;
	return true;
}

void CPlayer::ClearAttack()
{
	m_attackType = eAttackType::None;
	m_attackTargetObjectNum = UINT64_MAX;
	m_nextHitIndex = 0;
	if (m_state != ePlayerState::Die) m_state = ePlayerState::Idle;
}

void CPlayer::EnterView(CFieldObject* _object)
{
	if (_object == nullptr || _object == this) return;

	std::pair<std::set<CFieldObject*>::iterator, bool> result =
		m_visibleObjectList.insert(_object);
	if (!result.second) return;

	FIELD_VIEW_EVENT event;
	event.type = eFieldViewEventType::Enter;
	event.objectType = _object->GetObjectType();
	event.objectNum = _object->GetNum();
	m_viewEventList.push_back(event);
}

void CPlayer::LeaveView(CFieldObject* _object)
{
	if (_object == nullptr || _object == this) return;

	std::set<CFieldObject*>::iterator iter = m_visibleObjectList.find(_object);
	if (iter == m_visibleObjectList.end()) return;

	m_visibleObjectList.erase(iter);

	FIELD_VIEW_EVENT event;
	event.type = eFieldViewEventType::Leave;
	event.objectType = _object->GetObjectType();
	event.objectNum = _object->GetNum();
	m_viewEventList.push_back(event);
}

bool CPlayer::IsVisible(CFieldObject* _object)
{
	if (_object == nullptr) return false;
	return m_visibleObjectList.find(_object) != m_visibleObjectList.end();
}

eClassState CPlayer::GetClass()
{
	return m_class;
}

uint64_t CPlayer::GetCharacterIndex()
{
	return m_characterIndex;
}

std::string& CPlayer::GetName()
{
	return m_name;
}

ePlayerState CPlayer::GetState()
{
	return m_state;
}

uint64_t CPlayer::GetMaxHP()
{
	return m_maxHP;
}

uint64_t CPlayer::GetHP()
{
	return m_hp;
}

uint64_t CPlayer::GetLevel()
{
	return m_level;
}

uint64_t CPlayer::GetExperience()
{
	return m_experience;
}

uint64_t CPlayer::GetRequiredExperience()
{
	if (m_level >= 20) return 0;
	const PLAYER_LEVEL_STAT* stat = nullptr;
	if (m_playerStatDB != nullptr) stat = m_playerStatDB->Find(m_class, m_level);
	if (stat != nullptr) return stat->requiredExperience;
	return m_level * m_level * 100;
}

eAttackType CPlayer::GetAttackType()
{
	return m_attackType;
}
uint64_t CPlayer::GetAttackTargetObjectNum()
{
	return m_attackTargetObjectNum;
}
bool CPlayer::ConsumeRapidHit(uint8_t _hitIndex)
{
	if (m_attackType != eAttackType::RapidFire) return false;
	if (_hitIndex != m_nextHitIndex || m_nextHitIndex >= 20) return false;
	++m_nextHitIndex;
	return true;
}

VECTOR3& CPlayer::GetDestPosition()
{
	return m_destPosition;
}

std::set<CFieldObject*>& CPlayer::GetVisibleObjectList()
{
	return m_visibleObjectList;
}

std::vector<FIELD_VIEW_EVENT>& CPlayer::GetViewEventList()
{
	return m_viewEventList;
}

void CPlayer::ClearViewEventList()
{
	m_viewEventList.clear();
}
