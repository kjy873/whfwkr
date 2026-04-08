#pragma once

#include <memory>
#include <atomic>
#include <unordered_map>
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
	bool isDead = false;

public:
	uint64 GetObjectId() const
	{
		return playerInfo ? playerInfo->object_id() : 0;
	}

	void OnDamaged(int32 damage);

public:
	bool CanUseSkill(int32 skillId) const;
	void MarkSkillUsed(int32 skillId);
	float GetSkillCooldown(int32 skillId) const;

private:
	std::unordered_map<int32, double> _lastSkillUseTime;
};

