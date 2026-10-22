#include "Player.h"

void CPlayer::Initialize(uint64_t _characterIndex, std::string _name, uint64_t _level, eJobClass _class) noexcept
{
	m_characterIndex = _characterIndex;
	m_name.swap(_name);
	m_level = _level;
	m_class = _class;
}

void CPlayer::Cleanup() noexcept
{
	m_characterIndex = 0;
	m_name.clear();
	m_level = 0;
	m_class = eJobClass::Warrior;
}


void CPlayer::UpdateLevel(uint64_t _level) noexcept
{
	m_level = _level;
}


uint64_t CPlayer::GetCharacterIndex() const noexcept
{
	return m_characterIndex;
}

const std::string& CPlayer::GetName() const noexcept
{
	return m_name;
}

eJobClass CPlayer::GetClass() const noexcept
{
	return m_class;
}

uint64_t CPlayer::GetLevel() const noexcept
{
	return m_level;
}
