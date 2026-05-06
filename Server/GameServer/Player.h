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
	int32 Hp = 100;
	int32 MaxHp = 100;
	bool isDead = false;

	int32 KillCount = 0;
	int32 DeathCount = 0;
	int32 MonsterKillCount = 0;

public:
	uint64 GetObjectId() const
	{
		return playerInfo ? playerInfo->object_id() : 0;
	}

	void OnDamaged(PlayerRef attacker, int32 damage);

public:
	bool CanUseSkill(int32 skillId) const;
	void MarkSkillUsed(int32 skillId);
	float GetSkillCooldown(int32 skillId) const;

private:
	std::unordered_map<int32, double> _lastSkillUseTime;
};

