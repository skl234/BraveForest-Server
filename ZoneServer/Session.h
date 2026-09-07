#pragma once
#include <WinSock2.h>
#include <cstdint>

class CPlayer;

class CSession
{
private:
	uint64_t	m_sessionId = 0;
	uint64_t	m_accountIndex = 0;
	uint64_t	m_characterIndex = 0;
	uint64_t	m_channelId = 0;
	CPlayer*	m_player = nullptr;
	bool		m_active = false;
	SRWLOCK		m_lock;

public:
	CSession();
	~CSession() = default;

	void Activate(uint64_t _sessionId, uint64_t _accountIndex);
	void SetPlayer(uint64_t _characterIndex, uint64_t _channelId, CPlayer* _player);
	void ClearPlayer();
	void Cleanup();

	uint64_t GetSessionId();
	uint64_t GetAccountIndex();
	uint64_t GetCharacterIndex();
	uint64_t GetChannelId();
	CPlayer* GetPlayer();
	bool IsActive();
};
