#include "pch.h"
#include "Room.h"
#include "Player.h"
#include "GameSession.h"
#include "ObjectUtils.h"
#include "RoomManager.h"
#include <thread>
#include <chrono>

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

}

void Room::Clear()
{
	WRITE_LOCK;

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

void Room::ApplySpawnByRoomType(PlayerRef player)
{
	if (player == nullptr || player->playerInfo == nullptr)
		return;

	float centerX = 0.f;
	float centerY = 0.f;
	float centerZ = 0.f;

	if (_roomType == RoomType::Lobby)
	{
		centerX = Utils::GetRandom(-260.f, 0.f);
		centerY = Utils::GetRandom(-90.f, 0.f);
		centerZ = 0.f;
	}
	else if (_roomType == RoomType::Hunting)
	{
		centerX = 40025.f;
		centerY = 47369.f;
		centerZ = -689.f;
	}
	else if (_roomType == RoomType::Battle)
	{
		centerX = 4275.f;
		centerY = 4184.f;
		centerZ = -1187.210769f;
	}

	player->playerInfo->set_x(centerX);
	player->playerInfo->set_y(centerY);
	player->playerInfo->set_z(centerZ);
	player->playerInfo->set_yaw(Utils::GetRandom(0.f, 360.f));

	cout << "[ApplySpawnByRoomType] objId=" << player->playerInfo->object_id()
		<< " roomType=" << static_cast<int>(_roomType)
		<< " pos=(" << centerX << ", " << centerY << ", " << centerZ << ")" << endl;
}

bool Room::HandleEnterPlayerLocked(PlayerRef player, RoomRef self)
{
	WRITE_LOCK;

	if (EnterPlayer(player, self) == false)
	{
		cout << "[SERVER] EnterPlayer Failed for object_id = " << player->playerInfo->object_id() << endl;
		return false;
	}

	/*
	ApplySpawnByRoomType(player);
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
	*/
	return true;
}

void Room::HandleClientLevelReady(PlayerRef player)
{
	WRITE_LOCK;

	if (player == nullptr || player->playerInfo == nullptr)
		return;

	if (_roomType == RoomType::Battle || _roomType == RoomType::Hunting)
	{
		ApplySpawnByRoomType(player);
	}

	else if (player->playerInfo->x() == 0.f &&
		player->playerInfo->y() == 0.f &&
		player->playerInfo->z() == 0.f)
	{
		ApplySpawnByRoomType(player);
	}

	cout << "[HandleClientLevelReady] objId="
		<< player->playerInfo->object_id()
		<< " roomType=" << static_cast<int>(_roomType)
		<< " pos=("
		<< player->playerInfo->x() << ", "
		<< player->playerInfo->y() << ", "
		<< player->playerInfo->z() << ")" << endl;

	{
		Protocol::S_ENTER_GAME enterGamePkt;
		enterGamePkt.set_success(true);

		auto* selfInfo = new Protocol::PlayerInfo();
		selfInfo->CopyFrom(*player->playerInfo);
		enterGamePkt.set_allocated_player(selfInfo);

		if (auto session = player->session.lock())
			session->Send(ServerPacketHandler::MakeSendBuffer(enterGamePkt));
	}

	{
		Protocol::S_SPAWN spawnPkt;

		for (auto& item : _players)
		{
			if (item.first == player->playerInfo->object_id())
				continue;

			if (item.second == nullptr || item.second->playerInfo == nullptr)
				continue;

			if (item.second->playerInfo->x() == 0.f &&
				item.second->playerInfo->y() == 0.f &&
				item.second->playerInfo->z() == 0.f)
			{
				continue;
			}

			auto* info = spawnPkt.add_players();
			info->CopyFrom(*item.second->playerInfo);
		}

		if (spawnPkt.players_size() > 0)
		{
			if (auto session = player->session.lock())
				session->Send(ServerPacketHandler::MakeSendBuffer(spawnPkt));
		}
	}

	{
		Protocol::S_SPAWN broadcastPkt;
		auto* broadcastInfo = broadcastPkt.add_players();
		broadcastInfo->CopyFrom(*player->playerInfo);

		Broadcast(ServerPacketHandler::MakeSendBuffer(broadcastPkt), player->playerInfo->object_id());
	}
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
	{
		return;
	}

	PlayerRef& player = _players[objectId];
	player->playerInfo->CopyFrom(pkt.info());

	Protocol::S_MOVE movePkt;
	Protocol::PlayerInfo* info = movePkt.mutable_info();
	info->CopyFrom(pkt.info());

	SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(movePkt);
	Broadcast(sendBuffer);
}

bool Room::IsPvp() const
{
	return _roomType == RoomType::Battle;
}

void Room::CheckBattleEnd()
{
	vector<PlayerRef> alivePlayers;

	{
		WRITE_LOCK;

		if (_roomType != RoomType::Battle)
			return;

		for (auto& item : _players)
		{
			PlayerRef player = item.second;
			if (player && player->hp > 0)
				alivePlayers.push_back(player);
		}

		cout << "[CheckBattleEnd] aliveCount=" << alivePlayers.size() << endl;

		if (alivePlayers.size() >= 2)
			return;

		_isBattlePhase = false;
	}

	Protocol::S_CHANGE_LEVEL pkt;
	pkt.set_level_name("LandscapeMap");
	SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(pkt);
	Broadcast(sendBuffer);

	cout << "[Room] Broadcast S_CHANGE_LEVEL -> HuntingMap" << endl;

	for (auto& player : alivePlayers)
	{
		GRoomManager.MoveToHuntingRoom(player);
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

void Room::StartReturnToMap1Timer()
{
	WRITE_LOCK;

	if (bReturnToMap1TimerStarted)
		return;

	bReturnToMap1TimerStarted = true;

	RoomRef room = static_pointer_cast<Room>(shared_from_this());

	std::thread([room]()
		{
			std::this_thread::sleep_for(std::chrono::seconds(20));

			room->DoAsync([room]()
				{
					room->bReturnToMap1TimerStarted = false;

					vector<PlayerRef> playersToMove;
					for (auto& item : room->_players)
					{
						if (item.second)
							playersToMove.push_back(item.second);
					}

					for (auto& player : playersToMove)
					{
						GRoomManager.MoveToHuntingRoom(player);
					}
				});
		}).detach();
}

void Room::SendExistingPlayersTo(GameSessionRef session, uint64 excludeObjectId)
{
	WRITE_LOCK;

	Protocol::S_SPAWN spawnPkt;

	cout << "[Room::SendExistingPlayersTo] exclude=" << excludeObjectId
		<< " playerCount=" << _players.size() << endl;

	for (const auto& pair : _players)
	{
		PlayerRef other = pair.second;
		if (other == nullptr || other->playerInfo == nullptr)
			continue;

		if (other->playerInfo->object_id() == excludeObjectId)
			continue;

		spawnPkt.add_players()->CopyFrom(*other->playerInfo);
	}

	if (spawnPkt.players_size() > 0)
	{
		SendBufferRef spawnBuffer = ServerPacketHandler::MakeSendBuffer(spawnPkt);
		session->Send(spawnBuffer);
	}
}

void Room::BroadcastPlayerSpawn(PlayerRef player)
{
	Protocol::S_SPAWN pkt;
	pkt.add_players()->CopyFrom(*player->playerInfo);

	SendBufferRef buffer = ServerPacketHandler::MakeSendBuffer(pkt);
	Broadcast(buffer);
}

void Room::CheckAndStartGame()
{
	vector<PlayerRef> players;

	{
		WRITE_LOCK;

		if (_roomType != RoomType::Lobby)
			return;

		if (bGameCountdownStarted)
			return;

		if (_players.size() < 2)
			return;

		_isBattlePhase = true;

		bGameCountdownStarted = true;
	}

	RoomRef room = static_pointer_cast<Room>(shared_from_this());

	std::thread([room]()
		{
			std::this_thread::sleep_for(std::chrono::seconds(20));

			room->DoAsync([room]()
			{
					vector<PlayerRef> playersToMove;

					if (room->_players.size() < 2)
					{
						room->bGameCountdownStarted = false;
						cout << "[Room] Hunting start canceled: not enough players" << endl;
						return;
					}

					for (auto& item : room->_players)
					{
						if (item.second)
							playersToMove.push_back(item.second);
					}

					room->bGameCountdownStarted = false;

					GRoomManager.StartRound(playersToMove);

					for (auto& player : playersToMove)
					{
						GRoomManager.MoveToHuntingRoom(player);
					}
				});
		}).detach();
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
}

void Room::BroadcastUseSkill(uint64 playerId, uint32 skillId, uint64 targetId)
{
	Protocol::S_USE_SKILL pkt;
	pkt.set_playerid(playerId);
	pkt.set_skillid(skillId);
	pkt.set_targetid(targetId);

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
	pkt.set_object_id(targetId);
	pkt.set_damage(damage);

	Broadcast(ServerPacketHandler::MakeSendBuffer(pkt));

	if (target->hp <= 0)
	{
		Protocol::S_PLAYER_DEAD deadPkt;
		deadPkt.set_object_id(targetId);
		Broadcast(ServerPacketHandler::MakeSendBuffer(deadPkt));
	}
}

void Room::HandleAttackPlayerLocked(uint64 attackerId, uint64 targetId, uint32 skillId)
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
	case 0: damage = 20; break;
	case 1: damage = 35; break;
	}

	cout << "[Damage] attackerId=" << attackerId
		<< " targetId=" << targetId
		<< " beforeHp=" << target->hp
		<< " damage=" << damage << endl;

	target->hp = max(0, target->hp - damage);

	cout << "[Damage] afterHp=" << target->hp << endl;

	Protocol::S_DAMAGE_PLAYER pkt;
	pkt.set_object_id(targetId);
	pkt.set_damage(damage);
	Broadcast(ServerPacketHandler::MakeSendBuffer(pkt));

	if (target->hp <= 0)
	{
		cout << "[SERVER] Player Dead: " << targetId << endl;

		Protocol::S_PLAYER_DEAD deadPkt;
		deadPkt.set_object_id(targetId);
		Broadcast(ServerPacketHandler::MakeSendBuffer(deadPkt));

		if (_roomType != RoomType::Battle)
			return;

		vector<PlayerRef> playersToMove;
		for (auto& item : _players)
		{
			PlayerRef p = item.second;
			if (p && p->hp > 0)
				playersToMove.push_back(p);
		}

		cout << "[BattleEndCheck] aliveCount=" << playersToMove.size() << endl;

		if (playersToMove.size() <= 1)
		{
			cout << "[BattleEnd] Move survivors to HuntingRoom" << endl;

			for (auto& p : playersToMove)
			{
				GRoomManager.MoveToHuntingRoom(p);
			}

			Protocol::S_CHANGE_LEVEL changePkt;
			changePkt.set_level_name("LandscapeMap");
			SendBufferRef changeBuf = ServerPacketHandler::MakeSendBuffer(changePkt);

			for (auto& p : playersToMove)
			{
				if (auto session = p->session.lock())
					session->Send(changeBuf);
			}
		}
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

	SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(pkt);
	Broadcast(sendBuffer);
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