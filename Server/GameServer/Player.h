#pragma once

#include <memory>
#include <atomic>
#include "Protocol.pb.h"

class GameSession;
class Room;

class Player : public enable_shared_from_this<Player>
{
public:
	Player();
	virtual ~Player();

public:
	Protocol::PlayerInfo* playerInfo;
	weak_ptr<GameSession> session;

public:
	weak_ptr<Room> room;

public:
	int32 hp = 100;
	int32 maxHp = 100;
};

