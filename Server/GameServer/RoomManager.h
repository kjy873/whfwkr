#pragma once

class RoomManager
{
public:
	RoomManager();
	RoomRef CreateHuntingRoom(PlayerRef player);
	RoomRef GetOrCreateBattleRoom();
	void MoveToBattleRoom(PlayerRef player);
	void Update(float deltaTime);

	RoomRef GetBattleRoom() { return _battleRoom; }
	static RoomRef _battleRoom;

private:
	USE_LOCK;
	unordered_map<uint64, RoomRef> _rooms;
	uint64 _roomIdGenerator = 1;
};

extern RoomManager GRoomManager;

