#include "ZonePacketCreator.h"
#include "Player.h"
#include "Monster.h"
#include "../Common/PACKET.h"

void CZonePacketCreator::FillObjectInfo(CFieldObject* _object, FIELD_OBJECT_INFO& _info)
{
	_info = {};
	if (_object == nullptr) return;
	_info.objectNum = _object->GetNum();
	_info.position = {_object->GetPosition().x, 0.0f, _object->GetPosition().z};
	_info.rotationY = _object->GetRotationY();
	if (_object->GetObjectType() == eFieldObjectType::Player)
	{
		CPlayer* player = static_cast<CPlayer*>(_object);
		_info.objectType = eZoneObjectType::Player;
		_info.dataIndex = player->GetCharacterIndex();
		_info.jobClass = static_cast<eJobClass>(player->GetClass());
		_info.level = player->GetLevel();
		_info.maxHP = player->GetMaxHP();
		_info.hp = player->GetHP();
		_info.state = eZoneObjectState::Idle;
		_info.nameSize = static_cast<uint8_t>(player->GetName().size());
		if (_info.nameSize > 30) _info.nameSize = 30;
		if (_info.nameSize > 0) memcpy(_info.name, player->GetName().data(), _info.nameSize);
		if (player->GetState() == ePlayerState::Move) _info.state = eZoneObjectState::Move;
		if (player->GetState() == ePlayerState::Attack) _info.state = eZoneObjectState::Attack;
		if (player->GetState() == ePlayerState::Die) _info.state = eZoneObjectState::Dead;
		return;
	}
	CMonster* monster = static_cast<CMonster*>(_object);
	_info.objectType = eZoneObjectType::Monster;
	_info.dataIndex = static_cast<uint64_t>(monster->GetType());
	_info.level = monster->GetLevel();
	_info.maxHP = monster->GetMaxHP();
	_info.hp = monster->GetHP();
	_info.state = static_cast<eZoneObjectState>(monster->GetState());
}


std::shared_ptr<std::vector<byte>> CZonePacketCreator::CreateMonsterMove(const PROXY_HEADER& _header, CMonster* _monster)
{
	if (_monster == nullptr) return nullptr;
	std::vector<VECTOR3>& path = _monster->GetPath();
	uint64_t pathCount = static_cast<uint64_t>(path.size());
	uint64_t packetSize = sizeof(PACKET_S2C_MONSTER_MOVE) - sizeof(ZONE_VECTOR3) + sizeof(ZONE_VECTOR3) * pathCount;
	if (packetSize - sizeof(PROXY_HEADER) > UINT16_MAX) return nullptr;
	std::shared_ptr<std::vector<byte>> sendBuffer = std::make_shared<std::vector<byte>>(packetSize);
	PACKET_S2C_MONSTER_MOVE* send = reinterpret_cast<PACKET_S2C_MONSTER_MOVE*>(sendBuffer->data());
	send->proxyHeader = _header;
	send->packetHeader = {static_cast<uint16_t>(packetSize - sizeof(PROXY_HEADER)), ePacketType::S2C_MonsterMove};
	send->monsterObjectNum = _monster->GetNum();
	send->position = {_monster->GetPosition().x, 0.0f, _monster->GetPosition().z};
	send->rotationY = _monster->GetRotationY();
	send->moveSpeed = _monster->GetMoveSpeed();
	send->pathIndex = _monster->GetPathIndex();
	send->pathCount = pathCount;
	for (uint64_t i = 0; i < pathCount; ++i)
		send->path[i] = {path[i].x, 0.0f, path[i].z};
	return sendBuffer;
}
