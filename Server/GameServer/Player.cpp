#include "pch.h"
#include "Player.h"
#include "Room.h"

Player::Player()
{
	playerInfo = new Protocol::PlayerInfo();
}

Player::~Player()
{
	delete playerInfo;
}

void Player::OnDamaged(PlayerRef attacker, int32 damage)
{
	Hp -= damage;

	if (Hp <= 0)
	{
		DeathCount++;

		if (attacker != nullptr && attacker != shared_from_this())
		{
			attacker->KillCount++;
		}

		RoomRef room = room;
		if (room)
		{
			room->BroadcastPlayerStats(shared_from_this());

			if (attacker)
				room->BroadcastPlayerStats(attacker);
		}
	}
}

bool Player::CanUseSkill(int32 skillId) const
{
	double now = ::GetTickCount64() / 1000.0;

	auto it = _lastSkillUseTime.find(skillId);
	if (it == _lastSkillUseTime.end())
		return true;

	double lastTime = it->second;
	float cooldown = GetSkillCooldown(skillId);

	return (now - lastTime) >= cooldown;
}

void Player::MarkSkillUsed(int32 skillId)
{
	double now = ::GetTickCount64() / 1000.0;

	if (skillId == 0)
	{
		_iceShotCount++;

		if (_iceShotCount < 2)
			return;

		_iceShotCount = 0;
		_lastSkillUseTime[skillId] = now;
		return;
	}

	_lastSkillUseTime[skillId] = now;
}

float Player::GetSkillCooldown(int32 skillId) const
{
	switch (skillId)
	{
	case 0: return 0.5f; // Ice
	case 1: return 4.0f; // Fire
	default: return 1.0f;
	}
}