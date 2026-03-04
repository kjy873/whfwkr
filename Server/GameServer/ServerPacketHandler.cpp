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

	// 플레이어 생성 및 세션 연결
	PlayerRef player = ObjectUtils::CreatePlayer(gameSession);

	// 룸 생성 및 입장 (DoAsync는 CreateHuntingRoom 내부에서 수행하도록 설계)
	RoomRef room = GRoomManager.CreateHuntingRoom(player);

	if (room == nullptr) return false;

	// 응답 패킷 전송
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
	if (player == nullptr) return false;

	if (auto room = player->room.lock())
	{
		room->DoTimer(2000, &Room::HandleClientLevelReady, player);
	}

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

	PlayerRef player = gameSession->player.load();
	if (player == nullptr)
		return false;

	//RoomRef room = player->room.load().lock();
	RoomRef room = player->room.lock();
	if (room == nullptr)
		return false;

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
	uint64 playerId = pkt.playerid();
	uint32 skillId = pkt.skillid();
	cout << "[Server] C_USE_SKILL from " << playerId << " skill " << skillId << endl;

	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);
	PlayerRef player = gameSession->player.load();
	if (player == nullptr)
		return false;

	RoomRef room = player->room.lock();
	if (room == nullptr)
		return false;

	room->DoAsync(
		&Room::BroadcastUseSkill,
		player->playerInfo->object_id(),
		pkt.skillid()
	);

	return true;
}

bool Handle_C_ATTACK_PLAYER(PacketSessionRef& session, Protocol::C_ATTACK_PLAYER& pkt)
{
	GameSessionRef gs = static_pointer_cast<GameSession>(session);
	PlayerRef attacker = gs->player.load();
	if (!attacker) return false;

	RoomRef room = attacker->room.lock();
	if (!room) return false;

	room->DoAsync(
		&Room::HandleAttackPlayerLocked,
		attacker->playerInfo->object_id(),
		pkt.targetplayerid(),
		pkt.skillid()
	);

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
