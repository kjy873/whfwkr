#include "pch.h"
#include "Room.h"
#include "Player.h"
#include "GameSession.h"

RoomRef GRoom = make_shared<Room>();

Room::Room()
{
}

Room::~Room()
{
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

	cout << "[SERVER] EnterPlayer object_id = "<< player->playerInfo->object_id() << endl;

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

	
	return success;
}

bool Room::HandleLeavePlayerLocked(PlayerRef player)
{
	if (player == nullptr)
		return false;

	WRITE_LOCK;

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
	player->room = shared_from_this();


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
	for (auto& item : _players)
	{
		PlayerRef player = item.second;
		if (player->playerInfo->object_id() == exceptId)
			continue;

		if (GameSessionRef session = player->session.lock())
			session->Send(sendBuffer);
	}
}