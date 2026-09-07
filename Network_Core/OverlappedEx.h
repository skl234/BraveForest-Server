#pragma once
#include <WinSock2.h>
#include <memory>
#include <vector>

enum class eOverlappedType : byte
{
	Accept = 0,
	Recv,
	Send,
};

struct WSAOVERLAPPED_EX : public WSAOVERLAPPED
{
	eOverlappedType type;
	void*			owner;
	
	//Accept 요소
	//======================================
	SOCKET			socket;										
	byte			addrBuf[(sizeof(SOCKADDR_IN) + 16) * 2];	
	//======================================

	//Send 요소
	std::shared_ptr<std::vector<byte>> sendPacket;
};
