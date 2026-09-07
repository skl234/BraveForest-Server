#pragma once
#include <WinSock2.h>
#include "../Network_Core/TCPClient.h"

class CDummyClient : public CTCPClient
{
private:

public:
	CDummyClient() = default;
	~CDummyClient() override;

protected:
	void OnRecv() override;
	void OnError() override;
};
