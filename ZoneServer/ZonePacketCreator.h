#pragma once
#include <WinSock2.h>
#include <memory>
#include <vector>
#include "../Common/ZONE.h"
#include "../Common/PACKET_HEADER.h"

class CFieldObject;
class CMonster;

class CZonePacketCreator
{
public:
	static void									FillObjectInfo(CFieldObject* _object, FIELD_OBJECT_INFO& _info);

	static std::shared_ptr<std::vector<byte>>	CreateMonsterMove(const PROXY_HEADER& _header, CMonster* _monster);
};
