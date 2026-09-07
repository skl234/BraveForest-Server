#pragma once
#include <WinSock2.h>
#include <vector>
#include <string>
#include "Singleton.h"

enum class eLogLevel : byte
{
	eDEBUG = 0,
	eINFO,
	eWARN,
	eERROR,
};
	 
class CLogManager : public CSingleton<CLogManager>
{
	friend class CSingleton<CLogManager>;

private:
	std::vector<std::string>	m_levelList;
	std::vector<std::wstring>	m_wLevelList;
	FILE*						m_file;
	SRWLOCK						m_lock;

private:
	CLogManager();
	~CLogManager();

public:
	bool Initalize(const std::string& _path);

	bool Write(const eLogLevel& _logLevel, const char* _format, ...);
	bool WriteW(const eLogLevel& _logLevel, const wchar_t* _format, ...);
};
