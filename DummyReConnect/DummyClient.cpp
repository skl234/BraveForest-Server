#include "DummyClient.h"
#include <iostream>
#include "DummyManager.h"
#include "DummyMonitor.h"

CDummyClient::~CDummyClient()
{
	Disconnect();
}

void CDummyClient::OnRecv()
{
	m_recvBuffer.Cleanup(); //당장에 뭐 읽고 할건 없으니 밀어주기

	CDummyMonitor::GetInstance().AddRecvCount();

	if (!Recv()) Disconnect();
}

void CDummyClient::OnError()
{

}
