#include "MonitorApp.h"
#include <iostream>
#include <string>
#include "../Common/PACKET.h"
#include "../Utility_Core/MemoryStream.h"
#include "../Common/PACKET.h"
#include "../Network_Core/IOCP_TPS.h"
#include "../Utility_Core/JOB_TPS.h"

CMonitorApp::CMonitorApp()
{
}

bool CMonitorApp::Initialize()
{
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) { std::cout << "WSAStartup Fail : " << WSAGetLastError() << "\n"; return false; }
	
	return true;
}

void CMonitorApp::Run()
{
	m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_socket == INVALID_SOCKET) return;

	m_addr = {};
	m_addr.sin_family = AF_INET;
	m_addr.sin_port = htons(30002);
	inet_pton(AF_INET, "127.0.0.1", &m_addr.sin_addr);

	if (SOCKET_ERROR == connect(m_socket, reinterpret_cast<sockaddr*>(&m_addr), sizeof(m_addr)))
	{
		closesocket(m_socket);
		return;
	}
	
	PACKET_C2S_PROXY_MONITOR packet;
	while (true)
	{
		send(m_socket, reinterpret_cast<char*>(&packet), sizeof(packet), 0);

		m_offset = 0;
		while (true)
		{
			if (m_offset < 2) m_offset += recv(m_socket, m_recvBuff + m_offset, 2048 - m_offset, 0);
			else
			{
				if (m_offset < *reinterpret_cast<uint16_t*>(m_recvBuff)) m_offset += recv(m_socket, m_recvBuff + m_offset, 2048 - m_offset, 0);
				else break;
			}
		}
		
		//이곳에 이제 모니터정보 출력
		ShowMonitor();

		Sleep(1000);
	}
}

void CMonitorApp::Stop()
{
}

void CMonitorApp::ShowMonitor()
{
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	CONSOLE_CURSOR_INFO cursorInfo;
	GetConsoleCursorInfo(hConsole, &cursorInfo);
	cursorInfo.bVisible = FALSE;
	SetConsoleCursorInfo(hConsole, &cursorInfo);

	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(hConsole, &csbi);

	short width = csbi.dwSize.X;
	short height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

	size_t offset = 0;

	PACKET_HEADER* header =
		reinterpret_cast<PACKET_HEADER*>(m_recvBuff);
	offset += sizeof(PACKET_HEADER);

	uint64_t connectionCount =
		*reinterpret_cast<uint64_t*>(m_recvBuff + offset);
	offset += sizeof(uint64_t);

	uint16_t tpsCount =
		*reinterpret_cast<uint16_t*>(m_recvBuff + offset);
	offset += sizeof(uint16_t);

	IOCP_TPS* tpsList =
		reinterpret_cast<IOCP_TPS*>(m_recvBuff + offset);
	offset += sizeof(IOCP_TPS) * tpsCount;

	uint64_t jobCount =
		*reinterpret_cast<uint64_t*>(m_recvBuff + offset);
	offset += sizeof(uint64_t);

	uint16_t jobTpsCount =
		*reinterpret_cast<uint16_t*>(m_recvBuff + offset);
	offset += sizeof(uint16_t);

	JOB_TPS* jobTpsList =
		reinterpret_cast<JOB_TPS*>(m_recvBuff + offset);
	offset += sizeof(JOB_TPS) * jobTpsCount;

	uint64_t jobPoolSize = 
		*reinterpret_cast<uint64_t*>(m_recvBuff + offset);
	offset += sizeof(uint64_t);

	uint64_t totalTPS = 0;
	uint64_t totalRecv = 0;
	uint64_t totalSend = 0;
	uint64_t totalAccept = 0;
	uint64_t totalJobTPS = 0;

	for (uint16_t i = 0; i < tpsCount; ++i)
	{
		totalTPS += tpsList[i].total;
		totalRecv += tpsList[i].recvCount;
		totalSend += tpsList[i].sendCount;
		totalAccept += tpsList[i].acceptCount;
	}

	for (uint16_t i = 0; i < jobTpsCount; ++i)
	{
		totalJobTPS += jobTpsList[i].tps;
	}
	
	m_sendPending += totalJobTPS;
	m_sendPending -= totalSend;

	std::vector<std::string> lines;

	lines.push_back("==============================================================");
	lines.push_back("                        PROXY MONITOR");
	lines.push_back("==============================================================");
	lines.push_back("");

	lines.push_back("Connection                 : " + std::to_string(connectionCount));
	lines.push_back("Job Count                  : " + std::to_string(jobCount));
	lines.push_back("Total IOCP GQCS TPS        : " + std::to_string(totalTPS));
	lines.push_back("Total Recv Complete TPS    : " + std::to_string(totalRecv));
	lines.push_back("Total Send Complete TPS    : " + std::to_string(totalSend));
	lines.push_back("Total Accept Complete TPS  : " + std::to_string(totalAccept));
	lines.push_back("Total Job Excute TPS       : " + std::to_string(totalJobTPS));
	lines.push_back("Remaining Send Pending     : " + std::to_string(m_sendPending));
	lines.push_back("FreePacketHandlePoolSize   : " + std::to_string(jobPoolSize));
	lines.push_back("");

	lines.push_back("[IOCP THREAD TPS]");
	lines.push_back("ThreadID        Total        Recv         Send         Accept");
	lines.push_back("--------------------------------------------------------------");

	for (uint16_t i = 0; i < tpsCount; ++i)
	{
		char line[256];

		sprintf_s(
			line,
			sizeof(line),
			"%-15lu %-12llu %-12llu %-12llu %-12llu",
			tpsList[i].index,
			tpsList[i].total,
			tpsList[i].recvCount,
			tpsList[i].sendCount,
			tpsList[i].acceptCount
		);

		lines.push_back(line);
	}

	lines.push_back("");
	lines.push_back("[JOB THREAD TPS]");
	lines.push_back("ThreadID        TPS");
	lines.push_back("--------------------------------------------------------------");

	for (uint16_t i = 0; i < jobTpsCount; ++i)
	{
		char line[256];

		sprintf_s(
			line,
			sizeof(line),
			"%-15lu %-12llu",
			jobTpsList[i].threadId,
			jobTpsList[i].tps
		);

		lines.push_back(line);
	}

	DWORD written = 0;

	for (short y = 0; y < height; ++y)
	{
		std::string line;

		if (y < static_cast<short>(lines.size()))
		{
			line = lines[y];
		}

		if (line.size() > static_cast<size_t>(width))
		{
			line.resize(width);
		}
		else
		{
			line.append(width - line.size(), ' ');
		}

		COORD pos = { 0, y };

		WriteConsoleOutputCharacterA(
			hConsole,
			line.c_str(),
			static_cast<DWORD>(line.size()),
			pos,
			&written
		);
	}

	SetConsoleCursorPosition(hConsole, { 0, 0 });
}
