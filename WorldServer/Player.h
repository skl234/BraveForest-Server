#pragma once
#include <string>
#include "../Common/CHARACTER.h"

class CPlayer
{
private:
	uint64_t	m_characterIndex = 0;
	std::string	m_name;

	uint64_t	m_level = 0;
	eJobClass	m_class = eJobClass::Warrior;

public:
	CPlayer() = default;
	~CPlayer() = default;

	void				Initialize(uint64_t _characterIndex, std::string _name, uint64_t _level, eJobClass _class) noexcept;
	void				Cleanup() noexcept;

	void				UpdateLevel(uint64_t _level) noexcept;

	uint64_t			GetCharacterIndex() const noexcept;
	const std::string&	GetName() const noexcept;
	eJobClass			GetClass() const noexcept;
	uint64_t			GetLevel() const noexcept;
};
