#include "pch.h"
#include "GameSession.h"
#include "GameSessionManager.h"
#include "ServerPacketHandler.h"
#include "Room.h"
#include "Player.h"

void GameSession::OnConnected()
{
	GSessionManager.Add(static_pointer_cast<GameSession>(shared_from_this()));
}

void GameSession::OnDisconnected()
{
	// 플레이어가 있으면 Room에서 제거
	PlayerRef currentPlayer = player.load();
	if (currentPlayer != nullptr)
	{
		RoomRef room = currentPlayer->room.lock();
		if (room != nullptr)
		{
			room->DoAsync([room, currentPlayer]()
				{
					room->HandleLeavePlayerLocked(currentPlayer);
				});
		}
		player.store(nullptr);
	}

	GSessionManager.Remove(static_pointer_cast<GameSession>(shared_from_this()));
}

void GameSession::OnRecvPacket(BYTE* buffer, int32 len)
{
	PacketSessionRef session = GetPacketSessionRef();
	PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);

	ServerPacketHandler::HandlePacket(session, buffer, len);
}

void GameSession::OnSend(int32 len)
{
}