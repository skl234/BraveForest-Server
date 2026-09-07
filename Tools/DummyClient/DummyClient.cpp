#include "DummyClient.h"
#include <filesystem>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <conio.h>

static volatile LONG g_stopRequested = 0;

static BOOL WINAPI ConsoleHandler(DWORD _type)
{
	if (_type != CTRL_C_EVENT && _type != CTRL_BREAK_EVENT) return FALSE;
	InterlockedExchange(&g_stopRequested, 1);
	return TRUE;
}

void CDummyClient::Log(const std::string& _text)
{
	std::cout << _text << std::endl;
	if (m_log.is_open()) m_log << GetTickCount64() << " " << _text << std::endl;
}

bool CDummyClient::Initialize()
{
	if (!m_mapList[1].Load("Data/TownField.txt") || !m_mapList[2].Load("Data/BeginnerZoneField.txt")) return false;
	std::ifstream portals("Data/PortalExclusions.txt");
	uint64_t zoneId = 0;
	float x = 0, z = 0;
	uint64_t portalCount = 0;
	while (portals >> zoneId >> x >> z)
	{
		if (zoneId < 1 || zoneId > 2) return false;
		m_mapList[zoneId].AddPortal(x, z);
		++portalCount;
	}
	if (portalCount != 3) return false;
	char address[128]{};
	GetPrivateProfileStringA("Dummy", "IP", "192.168.219.100", address, sizeof(address), ".\\DummyClient.ini");
	m_ip = address;
	UINT port = GetPrivateProfileIntA("Dummy", "Port", 30002, ".\\DummyClient.ini");
	m_count = GetPrivateProfileIntA("Dummy", "Count", 500, ".\\DummyClient.ini");
	if (port == 0 || port > 65535 || m_count == 0 || m_count > 1000) return false;
	m_port = static_cast<uint16_t>(port);
	std::filesystem::create_directories("Logs");
	SYSTEMTIME time{};
	GetLocalTime(&time);
	char logPath[128]{};
	sprintf_s(logPath, "Logs/Dummy_%04u%02u%02u_%02u%02u%02u_%lu.log", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond, GetCurrentProcessId());
	m_log.open(logPath);
	return m_log.is_open();
}

bool CDummyClient::GenerateAccounts()
{
	uint64_t townCount = (m_count + 1) / 2;
	uint64_t beginnerCount = m_count - townCount;
	std::vector<ZONE_VECTOR3> spawns[3];
	for (uint64_t i = 1; i <= 2; ++i)
	{
		spawns[i] = m_mapList[i].GetSpawnList(m_random);
		Log("Zone " + std::to_string(i) + " usable spawn cells: " + std::to_string(spawns[i].size()));
		uint64_t required = townCount;
		if (i == 2) required = beginnerCount;
		if (spawns[i].size() < required) return false;
	}
	std::vector<uint64_t> classes(m_count, 0);
	for (uint64_t i = townCount; i < m_count; ++i) classes[i] = 1;
	std::shuffle(classes.begin(), classes.end(), m_random);
	std::ofstream file("Data/DummySpawn.txt");
	for (uint64_t i = 0; i < m_count; ++i)
	{
		uint64_t zone = 1;
		uint64_t spawnIndex = i;
		if (i >= townCount)
		{
			zone = 2;
			spawnIndex = i - townCount;
		}
		ZONE_VECTOR3 position = spawns[zone][spawnIndex];
		file << "Dummy" << std::setw(4) << std::setfill('0') << i + 1 << " " << classes[i] << " " << zone << " " << position.x << " " << position.z << "\n";
	}
	Log("Generated Data/DummySpawn.txt (no database changes).");
	return file.good();
}

bool CDummyClient::VerifyPaths()
{
	std::ifstream file("Data/DummySpawn.txt");
	std::string name;
	uint64_t job = 0, zone = 0, count = 0, found = 0, skipped = 0;
	ZONE_VECTOR3 anchor{};
	std::mt19937 random(20260907);
	while (file >> name >> job >> zone >> anchor.x >> anchor.z)
	{
		if (zone < 1 || zone > 2 || job > 1 || !m_mapList[zone].IsWalkable(anchor) || !m_mapList[zone].IsAwayFromPortal(anchor, 15)) return false;
		ZONE_VECTOR3 position = anchor;
		for (uint64_t i = 0; i < 100; ++i)
		{
			ZONE_VECTOR3 end = m_mapList[zone].RandomDestination(anchor, random);
			std::vector<ZONE_VECTOR3> path;
			if (CCellMap::Distance(end, anchor) >= 5) path = m_mapList[zone].FindPath(position, end, anchor);
			if (path.empty()) { ++skipped; continue; }
			++found;
			for (uint64_t j = 0; j < path.size(); ++j)
			{
				ZONE_VECTOR3 start = position;
				uint64_t steps = static_cast<uint64_t>(CCellMap::Distance(start, path[j]) * 100) + 1;
				for (uint64_t k = 1; k <= steps; ++k)
				{
					float t = static_cast<float>(k) / static_cast<float>(steps);
					position = { start.x + (path[j].x - start.x) * t, 0, start.z + (path[j].z - start.z) * t };
					if (!m_mapList[zone].IsInside(position, anchor)) return false;
				}
			}
			if (CCellMap::Distance(position, end) > .001f) return false;
		}
		++count;
	}
	Log("Path test: accounts=" + std::to_string(count) + " valid routes=" + std::to_string(found) + " skipped=" + std::to_string(skipped));
	return count == m_count && found > 0 && skipped > 0;
}

void CDummyClient::Send(DUMMY_CONNECTION& _connection, const void* _packet, size_t _size)
{
	if (_connection.socket == INVALID_SOCKET) return;
	if (_connection.sendBuffer.size() + _size > 1024 * 1024) { Close(_connection, "send queue overflow"); return; }
	const byte* data = static_cast<const byte*>(_packet);
	_connection.sendBuffer.insert(_connection.sendBuffer.end(), data, data + _size);
}

void CDummyClient::Close(DUMMY_CONNECTION& _connection, const std::string& _error)
{
	if (_connection.socket != INVALID_SOCKET)
	{
		shutdown(_connection.socket, SD_BOTH);
		closesocket(_connection.socket);
		_connection.socket = INVALID_SOCKET;
	}
	if (_error.empty()) _connection.state = eDummyState::Closed;
	else
	{
		Log("Dummy " + std::to_string(_connection.index) + " failed (state " + std::to_string(static_cast<uint64_t>(_connection.state)) + "): " + _error);
		_connection.state = eDummyState::Failed;
	}
	_connection.sendBuffer.clear();
}

void CDummyClient::StartConnect(DUMMY_CONNECTION& _connection, uint64_t _now)
{
	_connection.socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (_connection.socket == INVALID_SOCKET) { Close(_connection, "socket failed"); return; }
	u_long nonblocking = 1;
	ioctlsocket(_connection.socket, FIONBIO, &nonblocking);
	BOOL noDelay = TRUE;
	setsockopt(_connection.socket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<char*>(&noDelay), sizeof(noDelay));
	sockaddr_in address{};
	address.sin_family = AF_INET;
	address.sin_port = htons(m_port);
	if (inet_pton(AF_INET, m_ip.c_str(), &address.sin_addr) != 1) { Close(_connection, "invalid IP"); return; }
	_connection.state = eDummyState::Connecting;
	_connection.stateTime = _now;
	int result = connect(_connection.socket, reinterpret_cast<sockaddr*>(&address), sizeof(address));
	if (result == 0) Connected(_connection, _now);
	else if (WSAGetLastError() != WSAEWOULDBLOCK) Close(_connection, "connect failed: " + std::to_string(WSAGetLastError()));
}

void CDummyClient::Connected(DUMMY_CONNECTION& _connection, uint64_t _now)
{
	wchar_t id[32]{};
	swprintf_s(id, L"Dummy%04llu", _connection.index);
	std::wstring text = std::wstring(id) + L"1234";
	std::vector<byte> buffer(sizeof(PACKET_C2S_LOGIN) - 1 + text.size() * sizeof(wchar_t));
	PACKET_C2S_LOGIN* packet = reinterpret_cast<PACKET_C2S_LOGIN*>(buffer.data());
	packet->header = { static_cast<uint16_t>(buffer.size()), ePacketType::C2S_Login };
	packet->idSize = static_cast<int>(wcslen(id));
	packet->pwSize = 4;
	memcpy(packet->text, text.data(), text.size() * sizeof(wchar_t));
	Send(_connection, buffer.data(), buffer.size());
	_connection.state = eDummyState::Login;
	_connection.stateTime = _now;
}

void CDummyClient::FlushSend(DUMMY_CONNECTION& _connection)
{
	while (_connection.socket != INVALID_SOCKET && _connection.sendOffset < _connection.sendBuffer.size())
	{
		int result = send(_connection.socket, reinterpret_cast<char*>(_connection.sendBuffer.data() + _connection.sendOffset), static_cast<int>(_connection.sendBuffer.size() - _connection.sendOffset), 0);
		if (result > 0) _connection.sendOffset += result;
		else
		{
			if (WSAGetLastError() != WSAEWOULDBLOCK) Close(_connection, "send failed: " + std::to_string(WSAGetLastError()));
			return;
		}
	}
	_connection.sendBuffer.clear();
	_connection.sendOffset = 0;
}

void CDummyClient::Receive(DUMMY_CONNECTION& _connection, uint64_t _now)
{
	byte buffer[65536];
	// 한 연결이 다른 연결의 처리를 독점하지 않게 수신량 제한
	for (uint64_t readCount = 0; readCount < 16 && _connection.socket != INVALID_SOCKET; ++readCount)
	{
		int result = recv(_connection.socket, reinterpret_cast<char*>(buffer), sizeof(buffer), 0);
		if (result <= 0)
		{
			int error = 0;
			if (result < 0) error = WSAGetLastError();
			if (error == WSAEWOULDBLOCK) return;
			if ((result == 0 || error == WSAECONNRESET) && _connection.state == eDummyState::Logout && _connection.left && _connection.sendOffset == _connection.sendBuffer.size())
			{
				// 현재 Proxy는 Logout 응답을 전달하지 않고 소켓을 닫음(FIN/RST)
				// 입장 중/이동 중 RST는 실패. 퇴장 응답 + Logout 전송을 끝낸 경우만 인정
				if (error == WSAECONNRESET) ++m_logoutResetCount;
				_connection.loggedOut = true;
				Close(_connection);
			}
			else Close(_connection, "unexpected disconnect: " + std::to_string(error));
			return;
		}
		_connection.recvBuffer.insert(_connection.recvBuffer.end(), buffer, buffer + result);
		size_t offset = 0;
		while (offset + sizeof(PACKET_HEADER) <= _connection.recvBuffer.size())
		{
			PACKET_HEADER header{};
			memcpy(&header, _connection.recvBuffer.data() + offset, sizeof(header));
			if (header.size < sizeof(PACKET_HEADER)) { Close(_connection, "invalid packet size"); return; }
			if (offset + header.size > _connection.recvBuffer.size()) break;
			++m_rxPackets;
			ReadPacket(_connection, _connection.recvBuffer.data() + offset, header.size, _now);
			offset += header.size;
			if (_connection.socket == INVALID_SOCKET) return;
		}
		_connection.recvBuffer.erase(_connection.recvBuffer.begin(), _connection.recvBuffer.begin() + offset);
	}
}

void CDummyClient::ReadPacket(DUMMY_CONNECTION& _connection, const byte* _packet, uint16_t _size, uint64_t _now)
{
	PACKET_HEADER header{};
	memcpy(&header, _packet, sizeof(header));
	if (header.type == ePacketType::S2C_Login && _connection.state == eDummyState::Login)
	{
		if (_size != 5 || _packet[4] == 0) { Close(_connection, "login rejected"); return; }
		PACKET_C2S_CHARACTERLIST packet;
		Send(_connection, &packet, sizeof(packet));
		_connection.state = eDummyState::CharacterList;
		_connection.stateTime = _now;
	}
	else if (header.type == ePacketType::S2C_CharacterList && _connection.state == eDummyState::CharacterList)
	{
		if (_size != 5 + sizeof(CHARACTER) || _packet[4] != 1) { Close(_connection, "expected exactly one character"); return; }
		CHARACTER character{};
		memcpy(&character, _packet + 5, sizeof(character));
		_connection.characterIndex = character.index;
		PACKET_C2S_SELECT_CHARACTER packet;
		packet.characterIndex = _connection.characterIndex;
		Send(_connection, &packet, sizeof(packet));
		_connection.state = eDummyState::Select;
		_connection.stateTime = _now;
	}
	else if (header.type == ePacketType::S2C_SelectCharacter && _connection.state == eDummyState::Select)
	{
		if (_size != 5 + sizeof(PLAYER_DATA) || _packet[4] != 0) { Close(_connection, "select rejected"); return; }
		PLAYER_DATA data{};
		memcpy(&data, _packet + 5, sizeof(data));
		if (data.zoneId < 1 || data.zoneId > 2) { Close(_connection, "unsupported zone"); return; }
		_connection.zoneId = data.zoneId;
		PACKET_C2S_ENTER_ZONE packet;
		packet.characterIndex = _connection.characterIndex;
		Send(_connection, &packet, sizeof(packet));
		_connection.state = eDummyState::Enter;
		_connection.stateTime = _now;
	}
	else if (header.type == ePacketType::S2C_EnterZone && _connection.state == eDummyState::Enter)
	{
		constexpr size_t baseSize = sizeof(PACKET_S2C_ENTER_ZONE) - sizeof(PROXY_HEADER) - sizeof(FIELD_OBJECT_INFO);
		if (_size < baseSize || _packet[4] != 0) { Close(_connection, "enter rejected"); return; }
		uint64_t objectCount = 0, objectNum = 0;
		memcpy(&_connection.channelId, _packet + 5, 8);
		memcpy(&objectNum, _packet + 13, 8);
		memcpy(&objectCount, _packet + 37, 8);
		if (objectCount > 1000 || _size != baseSize + objectCount * sizeof(FIELD_OBJECT_INFO)) { Close(_connection, "invalid enter objects"); return; }
		bool found = false;
		for (uint64_t i = 0; i < objectCount; ++i)
		{
			FIELD_OBJECT_INFO info{};
			memcpy(&info, _packet + baseSize + i * sizeof(info), sizeof(info));
			if (info.objectType != eZoneObjectType::Player || info.objectNum != objectNum) continue;
			_connection.position = info.position;
			_connection.position.y = 0;
			_connection.anchor = _connection.position;
			_connection.rotationY = info.rotationY;
			found = true;
			break;
		}
		if (!found || !m_mapList[_connection.zoneId].IsInside(_connection.position, _connection.anchor)) { Close(_connection, "unsafe entry position"); return; }
		_connection.state = eDummyState::Online;
		_connection.entered = true;
		_connection.lastMoveTime = _now;
		_connection.nextMoveTime = _now + 200;
		_connection.nextWanderTime = _now + std::uniform_int_distribution<uint64_t>(3000, 10000)(m_random);
		if (m_stopping) BeginLogout(_connection, _now);
	}
	else if (header.type == ePacketType::S2C_LeaveZone && _connection.state == eDummyState::Leave)
	{
		if (_size != 5 || _packet[4] != 0) { Close(_connection, "leave rejected"); return; }
		_connection.left = true;
		PACKET_C2S_LOGOUT packet;
		Send(_connection, &packet, sizeof(packet));
		_connection.state = eDummyState::Logout;
		_connection.stateTime = _now;
	}
	else if (header.type == ePacketType::S2C_Logout && _connection.state == eDummyState::Logout)
	{
		if (_size != 5 || _packet[4] == 0) { Close(_connection, "logout rejected"); return; }
		_connection.loggedOut = true;
		Close(_connection);
	}
	else
	{
		// 화면을 그리지 않으므로 필드 오브젝트 정보 등은 보관하지 않음
		if (header.type == ePacketType::S2C_Chat) ++m_receivedChatCount;
		++m_discardPackets;
	}
}

void CDummyClient::SendMove(DUMMY_CONNECTION& _connection, bool _changed)
{
	PACKET_C2S_MOVE packet;
	packet.position = _connection.position;
	packet.destination = _connection.position;
	if (_connection.pathIndex < _connection.path.size()) packet.destination = _connection.path[_connection.pathIndex];
	packet.rotationY = _connection.rotationY;
	packet.destinationChanged = _changed;
	Send(_connection, &packet, sizeof(packet));
	++m_movePackets;
}

void CDummyClient::TickMove(DUMMY_CONNECTION& _connection, uint64_t _now)
{
	float distance = static_cast<float>(_now - _connection.lastMoveTime) * .001f * 5.0f;
	_connection.lastMoveTime = _now;
	while (distance > 0 && _connection.pathIndex < _connection.path.size())
	{
		ZONE_VECTOR3 target = _connection.path[_connection.pathIndex];
		float remaining = CCellMap::Distance(_connection.position, target);
		float dx = target.x - _connection.position.x;
		float dz = target.z - _connection.position.z;
		if (remaining > .0001f) _connection.rotationY = std::atan2(dx, dz) * 57.2957795f;
		if (distance >= remaining)
		{
			_connection.position = target;
			distance -= remaining;
			++_connection.pathIndex;
			SendMove(_connection, true);
		}
		else
		{
			_connection.position.x += dx * distance / remaining;
			_connection.position.z += dz * distance / remaining;
			distance = 0;
		}
	}
	if (!_connection.path.empty() && _connection.pathIndex >= _connection.path.size())
	{
		_connection.path.clear();
		_connection.pathIndex = 0;
		_connection.nextWanderTime = _now + std::uniform_int_distribution<uint64_t>(3000, 10000)(m_random);
	}
	if (_connection.path.empty() && _now >= _connection.nextWanderTime)
	{
		CCellMap& map = m_mapList[_connection.zoneId];
		ZONE_VECTOR3 end = map.RandomDestination(_connection.anchor, m_random);
		// 막힌 목적지면 이번 이동은 쉬기. 다른 목적지를 재추첨하지 않음
		if (CCellMap::Distance(end, _connection.anchor) >= 5) _connection.path = map.FindPath(_connection.position, end, _connection.anchor);
		_connection.pathIndex = 0;
		_connection.nextWanderTime = _now + std::uniform_int_distribution<uint64_t>(3000, 10000)(m_random);
		if (_connection.path.empty()) ++m_skippedCount;
		else { ++m_wanderCount; SendMove(_connection, true); }
	}
	float radius = CCellMap::Distance(_connection.position, _connection.anchor);
	m_maxDistance = (std::max)(m_maxDistance, radius);
	if (!m_mapList[_connection.zoneId].IsInside(_connection.position, _connection.anchor)) { Close(_connection, "movement left allowed area"); return; }
	if (_now >= _connection.nextMoveTime)
	{
		// 멈춘 위치는 도착 패킷으로 이미 전달됨. 이동 중에만 0.2초 위치 갱신
		if (!_connection.path.empty()) SendMove(_connection, false);
		_connection.nextMoveTime = _now + 200;
	}
}

void CDummyClient::ChatRandom()
{
	if (m_stopping) return;
	std::vector<uint64_t> onlineList;
	for (uint64_t i = 0; i < m_connectionList.size(); ++i)
	{
		if (m_connectionList[i].state != eDummyState::Online) continue;
		if (m_connectionList[i].socket == INVALID_SOCKET) continue;
		onlineList.push_back(i);
	}
	// 접속 중인 더미 중 무작위 100명 선택
	std::shuffle(onlineList.begin(), onlineList.end(), m_random);
	uint64_t selectedCount = static_cast<uint64_t>(onlineList.size());
	if (selectedCount > 100) selectedCount = 100;
	const std::string text = u8"Dummy 채팅 테스트입니다";
	std::vector<byte> buffer(sizeof(PACKET_C2S_CHAT) - 1 + text.size());
	PACKET_C2S_CHAT* packet = reinterpret_cast<PACKET_C2S_CHAT*>(buffer.data());
	packet->header = { static_cast<uint16_t>(buffer.size()), ePacketType::C2S_Chat };
	packet->chatSize = static_cast<uint8_t>(text.size());
	memcpy(packet->text, text.data(), text.size());
	uint64_t count = 0;
	for (uint64_t i = 0; i < selectedCount; ++i)
	{
		DUMMY_CONNECTION& connection = m_connectionList[onlineList[i]];
		Send(connection, buffer.data(), buffer.size());
		if (connection.state == eDummyState::Online) ++count;
	}
	m_chatCount += count;
	Log("t: chat queued by " + std::to_string(count) + " random dummies (online=" + std::to_string(onlineList.size()) + ", limit=100).");
}

void CDummyClient::BeginLogout(DUMMY_CONNECTION& _connection, uint64_t _now)
{
	_connection.path.clear();
	_connection.pathIndex = 0;
	SendMove(_connection, true);
	PACKET_C2S_LEAVE_ZONE packet;
	Send(_connection, &packet, sizeof(packet));
	_connection.state = eDummyState::Leave;
	_connection.stateTime = _now;
}

void CDummyClient::StopAll(uint64_t _now)
{
	if (m_stopping) return;
	m_stopping = true;
	m_stopTime = _now;
	Log("q: stop movement -> final position -> leave acknowledgement -> logout -> server close.");
	for (uint64_t i = 0; i < m_connectionList.size(); ++i)
	{
		DUMMY_CONNECTION& connection = m_connectionList[i];
		if (connection.state == eDummyState::Online) BeginLogout(connection, _now);
		else if (connection.state == eDummyState::Waiting || connection.state == eDummyState::Connecting) Close(connection);
		// 이미 보낸 로그인/입장 요청은 응답을 처리한 뒤 같은 종료 순서 사용
	}
}

void CDummyClient::Report(bool _final)
{
	uint64_t online = 0, entered = 0, left = 0, logout = 0, failed = 0, town = 0, beginner = 0;
	uint64_t channels[3][11]{};
	for (uint64_t i = 0; i < m_connectionList.size(); ++i)
	{
		DUMMY_CONNECTION& connection = m_connectionList[i];
		if (connection.entered) ++entered;
		if (connection.left) ++left;
		if (connection.loggedOut) ++logout;
		if (connection.state == eDummyState::Failed) ++failed;
		if (connection.state != eDummyState::Online) continue;
		++online;
		if (connection.zoneId == 1) ++town;
		if (connection.zoneId == 2) ++beginner;
		if (connection.channelId <= 10) ++channels[connection.zoneId][connection.channelId];
	}
	std::string label = "STATUS";
	if (_final) label = "FINAL";
	std::ostringstream text;
	text << label << " online=" << online << " town=" << town << " beginner=" << beginner << " entered=" << entered << " left=" << left << " logout=" << logout << " failed=" << failed
		<< " move=" << m_movePackets << " routes=" << m_wanderCount << " skipped=" << m_skippedCount << " received=" << m_rxPackets << " discarded=" << m_discardPackets
		<< " chatSent=" << m_chatCount << " chatReceived=" << m_receivedChatCount << " logoutServerReset=" << m_logoutResetCount << " maxAnchorDistance=" << m_maxDistance;
	Log(text.str());
	if (online == m_count)
	{
		for (uint64_t z = 1; z <= 2; ++z)
		{
			std::string row = "Zone " + std::to_string(z) + " channels:";
			for (uint64_t c = 1; c <= 10; ++c) row += " " + std::to_string(channels[z][c]);
			Log(row);
		}
	}
}

int CDummyClient::Run(uint64_t _durationSeconds, bool _autoChat)
{
	WSADATA data{};
	if (WSAStartup(MAKEWORD(2, 2), &data) != 0) return 1;
	SetConsoleCtrlHandler(ConsoleHandler, TRUE);
	m_connectionList.resize(m_count);
	for (uint64_t i = 0; i < m_count; ++i) m_connectionList[i].index = i + 1;
	Log("DummyClient: " + m_ip + ":" + std::to_string(m_port) + " count=" + std::to_string(m_count) + " / t+Enter: random 100 chat / q+Enter: graceful exit");
	uint64_t startTime = GetTickCount64(), allOnlineTime = 0, nextReport = 0;
	bool chatSent = false;
	std::wstring command;
	std::vector<WSAPOLLFD> polls;
	std::vector<uint64_t> indices;
	DWORD consoleMode = 0;
	bool console = GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &consoleMode) != FALSE;
	while (true)
	{
		uint64_t now = GetTickCount64();
		if (InterlockedCompareExchange(&g_stopRequested, 0, 0) != 0) StopAll(now);
		if (console)
		{
			while (_kbhit())
			{
				wint_t key = _getwch();
				if (key == '\r')
				{
					std::cout << std::endl;
					if (command == L"t" || command == L"T") ChatRandom();
					if (command == L"q" || command == L"Q") StopAll(now);
					command.clear();
				}
				else if (key == 8) { if (!command.empty()) command.pop_back(); }
				else if (key >= 32 && key < 127 && command.size() < 16) { command += static_cast<wchar_t>(key); std::cout << static_cast<char>(key); }
			}
		}
		else
		{
			DWORD available = 0;
			HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
			if (PeekNamedPipe(input, nullptr, 0, nullptr, &available, nullptr) && available > 0)
			{
				char buffer[256]{};
				DWORD read = 0;
				if (ReadFile(input, buffer, (std::min)(available, 256ul), &read, nullptr))
				{
					for (DWORD i = 0; i < read; ++i)
					{
						if (buffer[i] == '\r') continue;
						if (buffer[i] == '\n')
						{
							if (command == L"t" || command == L"T") ChatRandom();
							if (command == L"q" || command == L"Q") StopAll(now);
							command.clear();
						}
						else if (command.size() < 16) command += static_cast<wchar_t>(buffer[i]);
					}
				}
			}
		}
		uint64_t pending = 0, online = 0, active = 0;
		for (uint64_t i = 0; i < m_count; ++i)
		{
			DUMMY_CONNECTION& connection = m_connectionList[i];
			if (connection.state == eDummyState::Online) ++online;
			if (connection.state >= eDummyState::Connecting && connection.state <= eDummyState::Enter) ++pending;
			if (connection.state == eDummyState::Online && !m_stopping) TickMove(connection, now);
			if (connection.socket == INVALID_SOCKET) continue;
			++active;
			if (connection.state != eDummyState::Online && now - connection.stateTime > 60000) Close(connection, "handshake/shutdown timeout");
			if (m_stopping && now - m_stopTime > 65000 && connection.socket != INVALID_SOCKET) Close(connection, "shutdown timeout");
		}
		if (!m_stopping && m_startedCount < m_count && pending < 64 && now >= m_nextConnectTime)
		{
			for (uint64_t i = 0; i < 10 && m_startedCount < m_count && pending < 64; ++i, ++pending)
			{
				DUMMY_CONNECTION& connection = m_connectionList[m_startedCount++];
				StartConnect(connection, now);
				if (connection.socket != INVALID_SOCKET) ++active;
			}
			m_nextConnectTime = now + 50;
		}
		if (online == m_count && allOnlineTime == 0) { allOnlineTime = now; Log("ALL ONLINE"); Report(false); }
		if (_autoChat && !chatSent && allOnlineTime != 0 && now - allOnlineTime >= 3000) { ChatRandom(); chatSent = true; }
		if (_durationSeconds > 0 && allOnlineTime != 0 && now - allOnlineTime >= _durationSeconds * 1000) StopAll(now);
		if (_durationSeconds > 0 && allOnlineTime == 0 && now - startTime > 120000) StopAll(now);
		if (now >= nextReport) { Report(false); nextReport = now + 5000; }
		if (m_stopping && active == 0) break;
		if (!m_stopping && m_startedCount == m_count && active == 0) break;
		polls.clear();
		indices.clear();
		for (uint64_t i = 0; i < m_count; ++i)
		{
			DUMMY_CONNECTION& connection = m_connectionList[i];
			if (connection.socket == INVALID_SOCKET) continue;
			SHORT events = POLLRDNORM;
			if (connection.state == eDummyState::Connecting || connection.sendOffset < connection.sendBuffer.size()) events |= POLLWRNORM;
			polls.push_back({ connection.socket, events, 0 });
			indices.push_back(i);
		}
		if (polls.empty()) { Sleep(10); continue; }
		int result = WSAPoll(polls.data(), static_cast<ULONG>(polls.size()), 10);
		if (result == SOCKET_ERROR) { Log("WSAPoll failed"); StopAll(now); Sleep(10); continue; }
		for (uint64_t i = 0; i < polls.size(); ++i)
		{
			DUMMY_CONNECTION& connection = m_connectionList[indices[i]];
			SHORT events = polls[i].revents;
			if (events == 0) continue;
			if (connection.state == eDummyState::Connecting)
			{
				int error = 0, length = sizeof(error);
				getsockopt(connection.socket, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&error), &length);
				if (error != 0 || (events & (POLLERR | POLLHUP | POLLNVAL))) { Close(connection, "connection rejected"); continue; }
				Connected(connection, now);
			}
			if (events & (POLLRDNORM | POLLHUP)) Receive(connection, now);
			if (connection.socket != INVALID_SOCKET && (events & POLLWRNORM)) FlushSend(connection);
			if (connection.socket != INVALID_SOCKET && (events & (POLLERR | POLLNVAL))) Close(connection, "socket poll error");
		}
	}
	Report(true);
	std::ofstream positions("Logs/LastPositions.txt");
	positions << std::setprecision(9);
	int exitCode = 0;
	for (uint64_t i = 0; i < m_count; ++i)
	{
		DUMMY_CONNECTION& connection = m_connectionList[i];
		positions << connection.index << " " << connection.zoneId << " " << connection.anchor.x << " " << connection.anchor.z << " " << connection.position.x << " " << connection.position.z << " " << connection.left << " " << connection.loggedOut << "\n";
		if (connection.state == eDummyState::Failed || (connection.entered && (!connection.left || !connection.loggedOut))) exitCode = 1;
	}
	SetConsoleCtrlHandler(ConsoleHandler, FALSE);
	WSACleanup();
	return exitCode;
}
