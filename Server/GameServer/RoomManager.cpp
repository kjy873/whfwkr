#include "pch.h"
#include "Player.h"
#include "GameSession.h"
#include "Room.h"
#include "RoomManager.h"

RoomManager GRoomManager;
RoomRef RoomManager::_battleRoom = nullptr;

RoomManager::RoomManager()
{
    cout << "RoomManager Constructed: " << this << endl;
}

void RoomManager::Update(float deltaTime)
{
	vector<RoomRef> rooms;
	{
		WRITE_LOCK;
		for (auto& item : _rooms) rooms.push_back(item.second);
	}
	for (RoomRef room : rooms)
		room->DoAsync(&Room::UpdateTick, deltaTime);
}

RoomRef RoomManager::CreateHuntingRoom(PlayerRef player)
{
    WRITE_LOCK;

    RoomRef room = make_shared<Room>(RoomType::Hunting);

    uint64 roomId = _roomIdGenerator++;
    _rooms[roomId] = room;

    room->Init();
    room->DoAsync(&Room::HandleEnterPlayerLocked, player, room);

    return room;
}

RoomRef RoomManager::GetOrCreateBattleRoom()
{
    WRITE_LOCK;

    if (_battleRoom == nullptr)
    {
        _battleRoom = make_shared<Room>(RoomType::Battle);
        _battleRoom->Init();

        _rooms[0] = _battleRoom;
    }

    return _battleRoom;
}

void RoomManager::MoveToBattleRoom(PlayerRef player)
{
    auto currentRoom = player->room.lock();
    if (currentRoom && currentRoom->GetRoomType() == RoomType::Battle)
        return;

    if (currentRoom)
    {
        currentRoom->DoAsync(&Room::HandleLeavePlayerLocked, player);
    }

    RoomRef battleRoom = GetOrCreateBattleRoom();

    Protocol::S_CHANGE_LEVEL pkt;
    pkt.set_level_name("GameMap");
    auto sendBuffer = ServerPacketHandler::MakeSendBuffer(pkt);
    if (auto session = player->session.lock())
        session->Send(sendBuffer);

    if (player->playerInfo != nullptr)
    {
        player->playerInfo->set_x(0.0f);
        player->playerInfo->set_y(0.0f);
        player->playerInfo->set_z(0.0f);

        cout << "[DEBUG] Reset Player " << player->playerInfo->object_id() << " Pos to (0,0,0) for BattleRoom" << endl;
    }

    battleRoom->DoAsync(&Room::HandleEnterPlayerLocked, player, battleRoom);
}