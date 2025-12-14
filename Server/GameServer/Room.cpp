#include "pch.h"
#include "Room.h"
#include "Player.h"
#include "GameSession.h"
#include "ObjectUtils.h"

RoomRef GRoom;

Room::Room()
{
}

Room::~Room()
{
}

void Room::Init()
{
	// DoTimer(100, &Room::UpdateTick);
}


void Room::UpdateTick()
{
	// Protocol::S_MOVE_MOB pkt;
	// 
	// for (auto& kv : _mobs)
	// {
	// 	MobRef mob = kv.second;
	// 
	// 	mob->x += Utils::GetRandom(-5.f, 5.f);
	// 	mob->y += Utils::GetRandom(-5.f, 5.f);
	// 
	// 	Protocol::MobInfo* info = pkt.add_mobs();
	// 	info->CopyFrom(mob->ToInfo());
	// }
	// 
	// if (pkt.mobs_size() > 0)
	// {
	// 	SendBufferRef send = ServerPacketHandler::MakeSendBuffer(pkt);
	// 	Broadcast(send);
	// }
	// 
	// DoTimer(100, &Room::UpdateTick);
}


void Room::Clear()
{
	WRITE_LOCK;

	// 모든 플레이어의 room 참조 해제
	for (auto& item : _players)
	{
		PlayerRef player = item.second;
		if (player != nullptr)
		{
			player->room.reset();
		}
	}

	_players.clear();
}

bool Room::HandleEnterPlayerLocked(PlayerRef player)
{
	WRITE_LOCK;

	cout << "[SERVER] EnterPlayer object_id = " << player->playerInfo->object_id() << endl;

	bool success = EnterPlayer(player);

	//player->playerInfo->set_x(Utils::GetRandom(0.f, 500.f));
	//player->playerInfo->set_y(Utils::GetRandom(0.f, 500.f));
	//player->playerInfo->set_z(100.f);
	//player->playerInfo->set_yaw(Utils::GetRandom(0.f, 100.f));

	float centerX = 40025.f;
	float centerY = 47369.f;
	float centerZ = -689.f;

	player->playerInfo->set_x(centerX);
	player->playerInfo->set_y(centerY);
	player->playerInfo->set_z(centerZ);
	player->playerInfo->set_yaw(Utils::GetRandom(0.f, 360.f));


	{
		Protocol::S_ENTER_GAME enterGamePkt;
		enterGamePkt.set_success(success);

		Protocol::PlayerInfo* playerInfo = new Protocol::PlayerInfo();
		playerInfo->CopyFrom(*player->playerInfo);
		enterGamePkt.set_allocated_player(playerInfo);

		SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(enterGamePkt);
		if (auto session = player->session.lock())
			session->Send(sendBuffer);
	}

	{
		Protocol::S_SPAWN spawnPkt;

		Protocol::PlayerInfo* playerInfo = spawnPkt.add_players();
		playerInfo->CopyFrom(*player->playerInfo);

		SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(spawnPkt);
		Broadcast(sendBuffer, player->playerInfo->object_id());
	}

	{
		// 새 플레이어에게 기존 플레이어들의 정보 전송
		Protocol::S_SPAWN spawnPkt;

		for (auto& item : _players)
		{
			if (item.first == player->playerInfo->object_id())
				continue;

			Protocol::PlayerInfo* info = spawnPkt.add_players();
			info->CopyFrom(*item.second->playerInfo);
		}

		if (spawnPkt.players_size() > 0)
		{
			SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(spawnPkt);
			if (auto session = player->session.lock())
				session->Send(sendBuffer);
		}
	}

	{
		// 새 플레이어에게 기존 몬스터들의 정보 전송
		Protocol::S_SPAWN_MOB spawnMobPkt;

		for (auto& item : _mobs)
		{
			Protocol::MobInfo* mobInfo = spawnMobPkt.add_mobs();
			mobInfo->CopyFrom(item.second->ToInfo());
		}

		if (spawnMobPkt.mobs_size() > 0)
		{
			cout << "[Room::HandleEnterPlayerLocked] Sending " << spawnMobPkt.mobs_size()
				<< " existing mobs to new player" << endl;
			SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(spawnMobPkt);
			if (auto session = player->session.lock())
				session->Send(sendBuffer);
		}
	}


	return success;
}

bool Room::HandleLeavePlayerLocked(PlayerRef player)
{
	WRITE_LOCK;

	if (player == nullptr)
		return false;

	const uint64 objectId = player->playerInfo->object_id();
	bool success = LeavePlayer(objectId);

	{
		Protocol::S_LEAVE_GAME leaveGamePkt;

		SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(leaveGamePkt);
		if (auto session = player->session.lock())
			session->Send(sendBuffer);
	}

	{
		Protocol::S_DESPAWN despawnPkt;
		despawnPkt.add_object_ids(objectId);

		SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(despawnPkt);
		Broadcast(sendBuffer, objectId);

		if (auto session = player->session.lock())
			session->Send(sendBuffer);
	}

	return success;
}

void Room::HandleMoveLocked(Protocol::C_MOVE& pkt)
{
	WRITE_LOCK;

	const uint64 objectId = pkt.info().object_id();
	if (_players.find(objectId) == _players.end())
		return;

	PlayerRef& player = _players[objectId];
	player->playerInfo->CopyFrom(pkt.info());

	{
		Protocol::S_MOVE movePkt;
		{
			Protocol::PlayerInfo* info = movePkt.mutable_info();
			info->CopyFrom(pkt.info());
		}
		SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(movePkt);
		Broadcast(sendBuffer);
	}
}

bool Room::EnterPlayer(PlayerRef player)
{
	if(_players.find(player->playerInfo->object_id()) != _players.end())
		return false;

	_players.insert(make_pair(player->playerInfo->object_id(), player));

	//player->room.store(shared_from_this());
	player->room = static_pointer_cast<Room>(JobQueue::shared_from_this());

	return true;
}

bool Room::LeavePlayer(uint64 objectId)
{
	if (_players.find(objectId) == _players.end())
		return false;

	PlayerRef player = _players[objectId];
	//player->room.store(weak_ptr<Room>());
	player->room.reset();

	_players.erase(objectId);

	return true;
}

void Room::Broadcast(SendBufferRef sendBuffer, uint64 exceptId)
{
	int32 sentCount = 0;
	for (auto& item : _players)
	{
		PlayerRef player = item.second;
		if (player->playerInfo->object_id() == exceptId)
			continue;

		if (GameSessionRef session = player->session.lock())
		{
			session->Send(sendBuffer);
			sentCount++;
		}
	}

	if (sentCount == 0)
	{
		cout << "[Room::Broadcast] WARNING: No players to send packet to!" << endl;
	}
	else
	{
		//cout << "[Room::Broadcast] Packet sent to " << sentCount << " player(s)" << endl;
	}
}

void Room::SpawnRandomMobs()
{
	WRITE_LOCK;

	uint64 id = ObjectUtils::GenerateId();

	MobRef mob = make_shared<Mob>();
	mob->mobId = id;
	mob->templateId = 1;

	mob->x = Utils::GetRandom(39000.f, 41000.f);
	mob->y = Utils::GetRandom(47000.f, 48000.f);
	mob->z = -689.f;

	mob->dirX = 1.f;
	mob->dirY = 0.f;
	mob->dirZ = 0.f;

	_mobs[id] = mob;

	Protocol::S_SPAWN_MOB pkt;
	Protocol::MobInfo* info = pkt.add_mobs();
	info->CopyFrom(mob->ToInfo());


	cout << "[Room::SpawnRandomMobs] Spawning Mob ID=" << id
		<< " at [" << mob->x << ", " << mob->y << ", " << mob->z << "]" << endl;

	cout << "[Room::SpawnRandomMobs] Player count: " << _players.size() << endl;

	SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(pkt);
	Broadcast(sendBuffer);

	cout << "[Room::SpawnRandomMobs] Packet broadcasted" << endl;
}


void Room::HandleMoveMobLocked(uint64 mobId, float x, float y, float z)
{
	WRITE_LOCK;

	auto it = _mobs.find(mobId);
	if (it == _mobs.end())
		return;

	MobRef mob = it->second;
	if (mob == nullptr)
		return;

	// 위치 갱신
	mob->x = x;
	mob->y = y;
	mob->z = z;

	// 클라들에게 이동 브로드캐스트
	Protocol::S_MOVE_MOB pkt;
	Protocol::MobInfo* info = pkt.mutable_mob();
	*info = mob->ToInfo();

	SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(pkt);

	Broadcast(sendBuffer);
}

void Room::HandleAttackMobLocked(uint64 playerId, uint64 mobId)
{
	WRITE_LOCK;

	auto mobIt = _mobs.find(mobId);
	if (mobIt == _mobs.end())
		return;

	MobRef mob = mobIt->second;
	if (mob == nullptr || mob->hp <= 0)
		return;

	const int32 damage = 10;
	mob->hp = max(0, mob->hp - damage);

	Protocol::S_DAMAGE_MOB damagePkt;
	damagePkt.set_mobid(mobId);
	damagePkt.set_damage(damage);
	damagePkt.set_hp(mob->hp);

	Broadcast(ServerPacketHandler::MakeSendBuffer(damagePkt));

	if (mob->hp <= 0)
	{
		Protocol::S_DESPAWN_MOB despawnPkt;
		despawnPkt.add_mobids(mobId);
		Broadcast(ServerPacketHandler::MakeSendBuffer(despawnPkt));
		_mobs.erase(mobId);
	}
}

void Room::BroadcastUseSkill(uint64 playerId, uint32 skillId)
{
	Protocol::S_USE_SKILL pkt;
	pkt.set_playerid(playerId);
	pkt.set_skillid(skillId);

	SendBufferRef send = ServerPacketHandler::MakeSendBuffer(pkt);
	Broadcast(send);
}
