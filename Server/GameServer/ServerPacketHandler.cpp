#include "pch.h"
#include "ServerPacketHandler.h"
#include "BufferReader.h"
#include "BufferWriter.h"
#include "GameSession.h"
#include "ObjectUtils.h"
#include "Room.h"
#include "Player.h"
#include "RoomManager.h"

PacketHandlerFunc GPacketHandler[UINT16_MAX];

bool Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);

	return false;
}

bool Handle_C_LOGIN(PacketSessionRef& session, Protocol::C_LOGIN& pkt)
{
	auto gameSession = std::static_pointer_cast<GameSession>(session);

	PlayerRef player = ObjectUtils::CreatePlayer(gameSession);

	RoomRef room = GRoomManager.GetOrCreateLobbyRoom();
	room->DoAsync(&Room::HandleEnterPlayerLocked, player, room);

	if (room == nullptr) return false;

	Protocol::S_LOGIN loginPkt;
	loginPkt.set_success(true);
	auto sendBuffer = ServerPacketHandler::MakeSendBuffer(loginPkt);
	session->Send(sendBuffer);

	return true;
}

bool Handle_C_ENTER_GAME(PacketSessionRef& session, Protocol::C_ENTER_GAME& pkt)
{
	auto gameSession = static_pointer_cast<GameSession>(session);

	PlayerRef player = gameSession->player.load();
	if (!player)
		return false;

	RoomRef room = player->room.lock();
	if (!room)
		return false;

	room->ApplySpawnByRoomType(player);

	cout << "[Handle_C_ENTER_GAME] objId=" << player->playerInfo->object_id()
		<< " roomType=" << static_cast<int>(room->GetRoomType())
		<< " pos=("
		<< player->playerInfo->x() << ", "
		<< player->playerInfo->y() << ", "
		<< player->playerInfo->z() << ")" << endl;

	room->DoAsync([room, player, gameSession]()
		{
			Protocol::S_ENTER_GAME enterPkt;
			enterPkt.mutable_player()->CopyFrom(*player->playerInfo);

			SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(enterPkt);
			gameSession->Send(sendBuffer);

			room->SendExistingPlayersTo(gameSession, player->playerInfo->object_id());
			room->BroadcastPlayerSpawn(player);

			if (room->GetRoomType() == RoomType::Lobby)
				room->CheckAndStartGame();
		});

	return true;
}

bool Handle_C_LEAVE_GAME(PacketSessionRef& session, Protocol::C_LEAVE_GAME& pkt)
{
	auto gameSession = static_pointer_cast<GameSession>(session);

	PlayerRef player = gameSession->player.load();
	if ( player == nullptr )
		return false;

	//RoomRef room = player->room.load().lock();
	RoomRef room = player->room.lock();
	if ( room == nullptr )
		return false;

	room->DoAsync([room, player]()
		{
			room->HandleLeavePlayerLocked(player);
		});

	return true;
}

bool Handle_C_MOVE(PacketSessionRef& session, Protocol::C_MOVE& pkt)
{

	auto gameSession = static_pointer_cast<GameSession>(session);
	if (gameSession == nullptr)
	{
		return false;
	}

	PlayerRef player = gameSession->player.load();
	if (player == nullptr)
	{
		return false;
	}

	RoomRef room = player->room.lock();
	if (room == nullptr)
	{
		return false;
	}

	Protocol::C_MOVE pktCopy = pkt;

	room->DoAsync([room, pktCopy]()
		{
			Protocol::C_MOVE pkt = pktCopy;
			room->HandleMoveLocked(pkt);
		});
	return true;
}

bool Handle_C_CHAT(PacketSessionRef& session, Protocol::C_CHAT& pkt)
{
	return true;
}

bool Handle_C_USE_SKILL(PacketSessionRef& session, Protocol::C_USE_SKILL& pkt)
{
	cout << "[Server] C_USE_SKILL received skillId=" << pkt.skillid() << endl;

	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);
	if (gameSession == nullptr)
		return false;

	PlayerRef attacker = gameSession->player;
	if (attacker == nullptr)
		return false;

	RoomRef room = attacker->room.lock();
	if (room == nullptr)
		return false;

	Protocol::S_USE_SKILL outPkt;
	outPkt.set_playerid(attacker->playerInfo->object_id());
	outPkt.set_skillid(pkt.skillid());
	outPkt.set_clientshotid(pkt.clientshotid());
	outPkt.set_projectileid(0); // 일단 없으면 0

	outPkt.mutable_spawnpos()->set_x(attacker->playerInfo->x());
	outPkt.mutable_spawnpos()->set_y(attacker->playerInfo->y());
	outPkt.mutable_spawnpos()->set_z(attacker->playerInfo->z());

	outPkt.mutable_dir()->set_x(pkt.dir().x());
	outPkt.mutable_dir()->set_y(pkt.dir().y());
	outPkt.mutable_dir()->set_z(pkt.dir().z());

	outPkt.set_servertick(0); // 나중에 틱 시스템 붙이면 실제 값

	SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(outPkt);
	room->Broadcast(sendBuffer);
	return true;
}

bool Handle_C_ATTACK_PLAYER(PacketSessionRef& session, Protocol::C_ATTACK_PLAYER& pkt)
{
	cout << "[Handle_C_ATTACK_PLAYER] target=" << pkt.targetplayerid()
		<< " skill=" << pkt.skillid() << endl;

	GameSessionRef gs = static_pointer_cast<GameSession>(session);
	PlayerRef attacker = gs->player;
	if (!attacker)
		return false;

	RoomRef room = attacker->room.lock();
	if (!room)
		return false;

	if (!room->IsPvp())
	{
		cout << "[PVP BLOCKED] RoomType is not Battle" << endl;
		return false;
	}

	room->DoAsync(
		&Room::HandleAttackPlayerLocked,
		attacker->playerInfo->object_id(),
		pkt.targetplayerid(),
		pkt.skillid()
	);

	return true;
}

bool Handle_C_LEVEL_READY(PacketSessionRef& session, Protocol::C_LEVEL_READY& pkt)
{
	auto gameSession = static_pointer_cast<GameSession>(session);

	PlayerRef player = gameSession->player.load();
	if (!player)
		return false;

	RoomRef room = player->room.lock();
	if (!room)
		return false;

	room->HandleClientLevelReady(player);
	return true;
}


//----------------------------------------------------------------------------------------------------------
//mob
bool Handle_C_ATTACK_MOB(PacketSessionRef& session, Protocol::C_ATTACK_MOB& pkt)
{
	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);
	PlayerRef player = gameSession->player.load();
	if (player == nullptr)
	{
		cout << "[Handle_C_ATTACK_MOB] Player is nullptr" << endl;
		return false;
	}

	RoomRef room = player->room.lock();
	if (room == nullptr)
	{
		cout << "[Handle_C_ATTACK_MOB] Room is nullptr" << endl;
		return false;
	}

	uint64 mobId = pkt.mobid();
	uint64 playerId = player->playerInfo->object_id();

	cout << "[Handle_C_ATTACK_MOB] Player " << playerId << " attacking Mob " << mobId << endl;

	room->DoAsync([room, playerId, mobId]()
		{
			room->HandleAttackMobLocked(playerId, mobId);
		});

	return true;
}

bool Handle_C_MOVE_MOB(PacketSessionRef& session, Protocol::C_MOVE_MOB& pkt)
{
	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);
	PlayerRef player = gameSession->player.load();
	if (player == nullptr)
		return false;

	RoomRef room = player->room.lock();
	if (room == nullptr)
		return false;

	uint64 mobId = pkt.mobid();

	const Protocol::Vector3& pos = pkt.pos();
	float x = pos.x();
	float y = pos.y();
	float z = pos.z();

	room->DoAsync([room, mobId, x, y, z]()
		{
			room->HandleMoveMobLocked(mobId, x, y, z);
		});

	return true;
}
