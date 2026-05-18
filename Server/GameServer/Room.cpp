#include "pch.h"
#include "Room.h"
#include "Player.h"
#include "GameSession.h"
#include "GameSessionManager.h"
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
		centerX = Utils::GetRandom(-260.f, 100.f);
		centerY = Utils::GetRandom(-90.f, 100.f);
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
		centerX = Utils::GetRandom(4275.f, 4800.f);
		centerY = Utils::GetRandom(4184.f, 4500.f);
		centerZ = -1187.210769f;
	}

	player->playerInfo->set_x(centerX);
	player->playerInfo->set_y(centerY);
	player->playerInfo->set_z(centerZ);
	player->playerInfo->set_yaw(Utils::GetRandom(0.f, 360.f));
}

void Room::HandleMonsterKill(uint64 playerId)
{
	WRITE_LOCK;

	auto it = _players.find(playerId);
	if (it == _players.end())
		return;

	PlayerRef player = it->second;
	if (player == nullptr)
		return;

	player->MonsterKillCount++;

	BroadcastPlayerStats(player);
}

bool Room::HandleEnterPlayerLocked(PlayerRef player, RoomRef self)
{
	WRITE_LOCK;

	if (EnterPlayer(player, self) == false)
	{
		cout << "[SERVER] EnterPlayer Failed for object_id = " << player->playerInfo->object_id() << endl;
		return false;
	}

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
			if (player && player->Hp > 0)
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
	PacketHeader* header = reinterpret_cast<PacketHeader*>(sendBuffer->Buffer());
	int32 sentCount = 0;

	for (auto& item : _players)
	{
		PlayerRef player = item.second;
		if (player == nullptr)
		{
			printf("[Room Broadcast] player null\n");
			continue;
		}

		uint64 objectId = player->playerInfo->object_id();

		if (objectId == exceptId)
		{
			printf("[Room Broadcast] skip except playerId=%llu\n", objectId);
			continue;
		}

		GameSessionRef session = player->session.lock();
		if (session == nullptr)
		{
			printf("[Room Broadcast] session null playerId=%llu\n", objectId);
			continue;
		}

		session->Send(sendBuffer);
		sentCount++;
	}
}

void Room::BroadcastUseSkill(uint64 playerId, uint32 skillId, float chargeScale)
{
	Protocol::S_USE_SKILL pkt;
	pkt.set_playerid(playerId);
	pkt.set_skillid(skillId);
	pkt.set_targetid(0);
	pkt.set_chargescale(chargeScale);

	SendBufferRef send = ServerPacketHandler::MakeSendBuffer(pkt);

	PacketHeader* header = reinterpret_cast<PacketHeader*>(send->Buffer());

	Broadcast(send, 0);
}

void Room::BroadcastStartSkillCharge(uint64 playerId, uint32 skillId)
{
	Protocol::S_START_SKILL_CHARGE pkt;
	pkt.set_playerid(playerId);
	pkt.set_skillid(skillId);

	SendBufferRef send = ServerPacketHandler::MakeSendBuffer(pkt);
	Broadcast(send);
}

void Room::BroadcastPlayerStats(PlayerRef player)
{
	if (player == nullptr || player->playerInfo == nullptr)
		return;

	GRoomManager.UpdatePlayerScore(player);

	uint64 objectId = player->playerInfo->object_id();

	Protocol::S_PLAYER_STATS pkt;

	pkt.set_object_id(objectId);
	pkt.set_kill_count(player->KillCount);
	pkt.set_death_count(player->DeathCount);
	pkt.set_monster_kill_count(player->MonsterKillCount);

	cout << "[BroadcastPlayerStats] ObjId=" << objectId
		<< " K=" << player->KillCount
		<< " D=" << player->DeathCount
		<< " M=" << player->MonsterKillCount
		<< endl;

	SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(pkt);
	GSessionManager.Broadcast(sendBuffer);
}

void Room::BroadcastGameResult()
{
	GRoomManager.BroadcastGameResult();
}

void Room::HandleAttackPlayerLocked(uint64 attackerId, uint64 targetId, uint32 skillId)
{
	WRITE_LOCK;

	cout << "[HandleAttackPlayerLocked ENTER]"
		<< " attackerId=" << attackerId
		<< " targetId=" << targetId
		<< " skillId=" << skillId
		<< " roomType=" << static_cast<int32>(_roomType)
		<< " players=" << _players.size()
		<< endl;

	auto attackerIt = _players.find(attackerId);
	auto targetIt = _players.find(targetId);

	if (attackerIt == _players.end())
	{
		cout << "[HandleAttackPlayerLocked BLOCK] attacker not found attackerId="
			<< attackerId << endl;
		return;
	}

	if (targetIt == _players.end())
	{
		cout << "[HandleAttackPlayerLocked BLOCK] target not found targetId="
			<< targetId << endl;
		return;
	}

	PlayerRef attacker = attackerIt->second;
	PlayerRef target = targetIt->second;

	if (attacker == nullptr || target == nullptr)
	{
		cout << "[HandleAttackPlayerLocked BLOCK] attacker or target null" << endl;
		return;
	}

	cout << "[HandleAttackPlayerLocked FOUND]"
		<< " attacker=" << attacker->playerInfo->object_id()
		<< " target=" << target->playerInfo->object_id()
		<< " targetHp=" << target->Hp
		<< " targetInfoHp=" << target->playerInfo->hp()
		<< endl;

	if (attacker == target)
	{
		cout << "[HandleAttackPlayerLocked BLOCK] self attack objId="
			<< attackerId << endl;
		return;
	}

	if (target->Hp <= 0)
	{
		cout << "[HandleAttackPlayerLocked BLOCK] target already dead"
			<< " targetId=" << targetId
			<< " Hp=" << target->Hp
			<< " playerInfoHp=" << target->playerInfo->hp()
			<< endl;
		return;
	}

	int32 damage = 0;

	switch (skillId)
	{
	case 0:
		damage = 20;
		break;

	case 1:
		damage = 35;
		break;

	default:
		damage = 0;
		break;
	}

	if (damage <= 0)
	{
		cout << "[HandleAttackPlayerLocked BLOCK] invalid damage skillId="
			<< skillId << endl;
		return;
	}

	const int32 PrevHp = target->Hp;

	target->Hp = max(0, target->Hp - damage);
	target->playerInfo->set_hp(target->Hp);

	cout << "[HandleAttackPlayerLocked DAMAGE]"
		<< " targetId=" << targetId
		<< " prevHp=" << PrevHp
		<< " damage=" << damage
		<< " newHp=" << target->Hp
		<< " newInfoHp=" << target->playerInfo->hp()
		<< endl;

	Protocol::S_DAMAGE_PLAYER damagePkt;
	damagePkt.set_object_id(targetId);
	damagePkt.set_damage(damage);

	Broadcast(ServerPacketHandler::MakeSendBuffer(damagePkt));

	if (target->Hp <= 0)
	{
		target->Hp = 0;
		target->playerInfo->set_hp(0);

		target->DeathCount++;

		if (attacker && attacker != target)
		{
			attacker->KillCount++;
		}

		cout << "[PlayerDead]"
			<< " targetId=" << targetId
			<< " DeathCount=" << target->DeathCount
			<< " attackerId=" << attackerId
			<< " attackerKill=" << (attacker ? attacker->KillCount : 0)
			<< endl;

		Protocol::S_PLAYER_DEAD deadPkt;
		deadPkt.set_object_id(targetId);

		Broadcast(ServerPacketHandler::MakeSendBuffer(deadPkt));

		if (attacker)
		{
			BroadcastPlayerStats(attacker);
			GRoomManager.UpdatePlayerScore(attacker);
		}

		if (target)
		{
			BroadcastPlayerStats(target);
			GRoomManager.UpdatePlayerScore(target);
		}
	}
}

void Room::HandleRespawnPlayerLocked(uint64 playerId)
{
	WRITE_LOCK;

	auto it = _players.find(playerId);
	if (it == _players.end())
	{
		cout << "[HandleRespawnPlayerLocked BLOCK] player not found playerId="
			<< playerId << endl;
		return;
	}

	PlayerRef player = it->second;
	if (player == nullptr || player->playerInfo == nullptr)
	{
		cout << "[HandleRespawnPlayerLocked BLOCK] player null playerId="
			<< playerId << endl;
		return;
	}

	if (_roomType != RoomType::Battle)
	{
		cout << "[HandleRespawnPlayerLocked BLOCK] not battle room playerId="
			<< playerId << endl;
		return;
	}

	if (player->Hp > 0)
	{
		cout << "[HandleRespawnPlayerLocked BLOCK] already alive playerId="
			<< playerId
			<< " Hp=" << player->Hp
			<< endl;
		return;
	}

	player->Hp = 50;
	player->playerInfo->set_hp(50);

	cout << "[HandleRespawnPlayerLocked OK] playerId="
		<< playerId
		<< " Hp=" << player->Hp
		<< " playerInfoHp=" << player->playerInfo->hp()
		<< " Pos=("
		<< player->playerInfo->x() << ", "
		<< player->playerInfo->y() << ", "
		<< player->playerInfo->z() << ")"
		<< endl;

	BroadcastPlayerStats(player);

	Protocol::S_SPAWN spawnPkt;
	Protocol::PlayerInfo* info = spawnPkt.add_players();
	info->CopyFrom(*player->playerInfo);

	SendBufferRef spawnBuffer = ServerPacketHandler::MakeSendBuffer(spawnPkt);

	cout << "[Respawn SendSpawnToAll] respawnPlayerId="
		<< playerId
		<< " roomPlayers=" << _players.size()
		<< endl;

	for (auto& item : _players)
	{
		PlayerRef p = item.second;
		if (p == nullptr || p->playerInfo == nullptr)
			continue;

		auto session = p->session.lock();
		if (!session)
		{
			cout << "[Respawn SendSpawnToAll BLOCK] session null objId="
				<< p->playerInfo->object_id()
				<< endl;
			continue;
		}

		cout << "[Respawn SendSpawnToAll TARGET] sendToObjId="
			<< p->playerInfo->object_id()
			<< " respawnObjId=" << playerId
			<< endl;

		session->Send(spawnBuffer);
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