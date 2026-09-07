#pragma once
#include <WinSock2.h>
#include <vector>
#include <deque>
#include <string>
#include <memory>
#include "DummyClient.h"
#include "../Utility_Core/Singleton.h"
#include "../Utility_Core/Thread.h"

class CDummyManager : public CSingleton<CDummyManager>
{
	friend class CSingleton<CDummyManager>;

private:
	std::vector<std::unique_ptr<CDummyClient>>	m_list;
	std::deque<CDummyClient*>					m_pool;
	SRWLOCK										m_lock;

private:
	CDummyManager();
	~CDummyManager();

public:
	void		  Initialize(uint32_t _size);
	CDummyClient* Acquire();
	void		  Release(CDummyClient* _client);

	void		  AllConnect(std::string _ip, uint16_t _port);

	uint64_t	  GetSize();
};

class CConnectThread : public CThread
{
protected:
	std::string m_ip;
	uint16_t	m_port;
	int			m_num;
	int			m_count;

	std::vector<std::unique_ptr<CDummyClient>>& m_list;

public:
	CConnectThread(std::vector<std::unique_ptr<CDummyClient>>& _list, std::string _ip, uint16_t _port, int _num, int _count) :
		m_list(_list),
		m_ip(_ip),
		m_port(_port),
		m_num(_num),
		m_count(_count){}
	~CConnectThread() override = default;

protected:
	void Run() override
	{
		uint64_t size = m_list.size();
		for (uint64_t i = m_num; i < size; i += m_count)
		{
			m_list[i]->Connect(m_ip, m_port);
		}
	}
};
