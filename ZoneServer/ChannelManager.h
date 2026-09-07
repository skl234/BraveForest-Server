#pragma once
#include <WinSock2.h>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include "Channel.h"

class CChannelManager
{
private:
	std::vector<std::unique_ptr<CChannel>>	m_channelList;

public:
	CChannelManager() = default;
	~CChannelManager() = default;

	bool Initialize(uint64_t _channelCount, uint64_t _playerPoolSize, const std::string& _fieldInfoPath, const std::vector<MONSTER_SPAWN_INFO>* _spawnInfoList = nullptr, const CPlayerStatDB* _playerStatDB = nullptr);
	bool Start();
	void Stop();
	void ScheduleMonster(uint64_t _currentTime);

	CChannel* FindChannel(uint64_t _channelId);
	CChannel* SelectEnterChannel();
	uint64_t GetChannelCount();
};
