#include "pch.h"
#include "Room.h"
#include "Player.h"
#include "GameSession.h"
#include "ObjectUtils.h"
#include "RoomManager.h"

Room::Room(RoomType type) : _roomType(type)
{
	_timer = 0.0f;
}

Room::~Room()
{
}

void Room::Init()
{

}

void Room::UpdateTick(float deltaTime)
{
	if (_roomType != RoomType::Hunting)
		return;

	_timer += deltaTime;

	if (_timer >= 10.0f)
	{
		_timer = 0.0f;

		cout << "[Room] 10 seconds passed! Moving players to BattleRoom" << endl;

		vector<PlayerRef> players;
		{
			WRITE_LOCK;
			for (auto& item : _players)
				players.push_back(item.second);
		}

		for (auto& p : players)
			GRoomManager.MoveToBattleRoom(p);
	}
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

bool Room::HandleEnterPlayerLocked(PlayerRef player, RoomRef self)
{
	WRITE_LOCK;

	if (EnterPlayer(player, self) == false)
	{
		cout << "[SERVER] EnterPlayer Failed for object_id = " << player->playerInfo->object_id() << endl;
		return false;
	}

	float centerX = 40025.f;
	float centerY = 47369.f;
	float centerZ = -689.f;

	player->playerInfo->set_x(centerX);
	player->playerInfo->set_y(centerY);
	player->playerInfo->set_z(centerZ);
	player->playerInfo->set_yaw(Utils::GetRandom(0.f, 360.f));
	player->playerInfo->set_hp(100);

	{
		Protocol::S_ENTER_GAME enterGamePkt;
		enterGamePkt.set_success(true);

		Protocol::PlayerInfo* playerInfo = new Protocol::PlayerInfo();
		playerInfo->CopyFrom(*player->playerInfo);
		enterGamePkt.set_allocated_player(playerInfo);

		SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(enterGamePkt);
		if (auto session = player->session.lock())
			session->Send(sendBuffer);
	}

	cout << "[SERVER] Player " << player->playerInfo->object_id() << " Entered Room. Waiting for Level Ready..." << endl;

	return true;
}

void Room::HandleClientLevelReady(PlayerRef player)
{
	WRITE_LOCK;
	if (player == nullptr || player->playerInfo == nullptr) return;

	uint64 myId = player->playerInfo->object_id();

	// _roomId나 _id 대신 현재 방의 타입이나 플레이어 수만 출력하도록 수정
	cout << "[DEBUG] Player " << myId << " Ready. Total Players in Room: " << _players.size() << endl;

	// 1. 나에게 '이미 로딩이 끝난' 다른 사람들의 정보를 보냅니다.
	Protocol::S_SPAWN spawnPkt;
	for (auto& item : _players)
	{
		if (item.first == myId) continue;

		// 상대방도 로딩이 끝난 상태인지 체크하는 플래그가 있다면 더 좋습니다.
		auto* info = spawnPkt.add_players();
		info->CopyFrom(*item.second->playerInfo);
	}

	if (spawnPkt.players_size() > 0)
	{
		auto sendBuffer = ServerPacketHandler::MakeSendBuffer(spawnPkt);
		player->session.lock()->Send(sendBuffer);
		cout << "[DEBUG] Sent " << spawnPkt.players_size() << " players info to Player " << myId << endl;
	}

	// 2. 다른 모든 사람들에게 '나'를 스폰하라고 방송합니다.
	Protocol::S_SPAWN broadcastPkt;
	auto* selfInfo = broadcastPkt.add_players();
	selfInfo->CopyFrom(*player->playerInfo);
	auto broadcastBuffer = ServerPacketHandler::MakeSendBuffer(broadcastPkt);

	// ★ 여기서 exceptId를 0으로 주거나, 자신을 포함해서 방송하여 
	// 최소한 로그상으로 sentCount가 0이 아니게 확인해보세요.
	Broadcast(broadcastBuffer, myId);
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

bool Room::EnterPlayer(PlayerRef player, RoomRef self)
{
	if (_players.find(player->playerInfo->object_id()) != _players.end())
		return false;

	_players.insert(make_pair(player->playerInfo->object_id(), player));

	player->room = self;

	return true;
}

bool Room::LeavePlayer(uint64 objectId)
{
	auto it = _players.find(objectId);
	if (it == _players.end())
		return false;

	PlayerRef player = it->second;

	if (auto cur = player->room.lock())
	{
		if (cur.get() == this)
			player->room.reset();
	}

	_players.erase(it);
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

void Room::BroadcastUseSkill(uint64 playerId, uint32 skillId)
{
	Protocol::S_USE_SKILL pkt;
	pkt.set_playerid(playerId);
	pkt.set_skillid(skillId);

	SendBufferRef send = ServerPacketHandler::MakeSendBuffer(pkt);
	Broadcast(send);
}

void Room::HandlePlayerHit(uint64 attackerId, uint64 targetId)
{
	WRITE_LOCK;

	auto it = _players.find(targetId);
	if (it == _players.end())
		return;

	PlayerRef target = it->second;
	if (target->hp <= 0)
		return;

	int32 damage = 20;
	target->hp -= damage;

	Protocol::S_DAMAGE_PLAYER pkt;
	pkt.set_playerid(targetId);
	pkt.set_damage(damage);

	Broadcast(ServerPacketHandler::MakeSendBuffer(pkt));

	if (target->hp <= 0)
	{
		Protocol::S_PLAYER_DEAD deadPkt;
		deadPkt.set_playerid(targetId);
		Broadcast(ServerPacketHandler::MakeSendBuffer(deadPkt));
	}
}

void Room::HandleAttackPlayerLocked(uint64 attackerId,uint64 targetId,uint32 skillId)
{
	WRITE_LOCK;

	auto attackerIt = _players.find(attackerId);
	auto targetIt = _players.find(targetId);
	if (attackerIt == _players.end() || targetIt == _players.end())
		return;

	PlayerRef attacker = attackerIt->second;
	PlayerRef target = targetIt->second;

	if (target->hp <= 0)
		return;

	int32 damage = 0;
	switch (skillId)
	{
	case 0: damage = 10; break; // 좌클릭
	case 1: damage = 25; break; // Q
	}

	target->hp = max(0, target->hp - damage);

	Protocol::S_DAMAGE_PLAYER pkt;
	pkt.set_playerid(targetId);
	pkt.set_damage(damage);

	Broadcast(ServerPacketHandler::MakeSendBuffer(pkt));

	if (target->hp <= 0)
	{
		cout << "[SERVER] Player Dead: " << targetId << endl;
	}
}


//-----------------------------------------------
// mob
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