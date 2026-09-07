#include "ChannelManager.h"

bool CChannelManager::Initialize(uint64_t _channelCount, uint64_t _playerPoolSize, const std::string& _fieldInfoPath, const std::vector<MONSTER_SPAWN_INFO>* _spawnInfoList, const CPlayerStatDB* _playerStatDB)
{
	if (_channelCount == 0) return false;
	if (_playerPoolSize == 0) return false;
	if (!m_channelList.empty()) return false;

	m_channelList.reserve(_channelCount);
	for (uint64_t i = 0; i < _channelCount; ++i)
	{
		std::unique_ptr<CChannel> channel = std::make_unique<CChannel>();
		if (!channel->Initialize(i + 1, _playerPoolSize, _fieldInfoPath, _spawnInfoList, _playerStatDB))
		{
			m_channelList.clear();
			return false;
		}
		m_channelList.emplace_back(std::move(channel));
	}
	return true;
}

bool CChannelManager::Start()
{
	uint64_t size = static_cast<uint64_t>(m_channelList.size());
	for (uint64_t i = 0; i < size; ++i)
	{
		if (!m_channelList[i]->Start())
		{
			for (uint64_t j = 0; j < i; ++j)
			{
				m_channelList[j]->Stop();
			}
			return false;
		}
	}
	return true;
}

void CChannelManager::Stop()
{
	uint64_t size = static_cast<uint64_t>(m_channelList.size());
	for (uint64_t i = 0; i < size; ++i)
	{
		m_channelList[i]->Stop();
	}
}

void CChannelManager::ScheduleMonster(uint64_t _currentTime)
{
	uint64_t size = static_cast<uint64_t>(m_channelList.size());
	for (uint64_t i = 0; i < size; ++i)
	{
		m_channelList[i]->ScheduleMonster(_currentTime);
	}
}

CChannel* CChannelManager::FindChannel(uint64_t _channelId)
{
	if (_channelId == 0) return nullptr;
	uint64_t index = _channelId - 1;
	if (index >= m_channelList.size()) return nullptr;
	return m_channelList[index].get();
}

CChannel* CChannelManager::SelectEnterChannel()
{
	CChannel* selectedChannel = nullptr;
	uint64_t size = static_cast<uint64_t>(m_channelList.size());
	for (uint64_t i = 0; i < size; ++i)
	{
		CChannel* channel = m_channelList[i].get();
		if (channel->GetPlayerCount() + channel->GetReservedPlayerCount() >= channel->GetMaxPlayerCount()) continue;
		if (selectedChannel == nullptr)
		{
			selectedChannel = channel;
			continue;
		}

		uint64_t channelCount = channel->GetPlayerCount() + channel->GetReservedPlayerCount();
		uint64_t selectedCount = selectedChannel->GetPlayerCount() + selectedChannel->GetReservedPlayerCount();
		if (channelCount < selectedCount)
		{
			selectedChannel = channel;
		}
	}
	return selectedChannel;
}

uint64_t CChannelManager::GetChannelCount()
{
	return static_cast<uint64_t>(m_channelList.size());
}
