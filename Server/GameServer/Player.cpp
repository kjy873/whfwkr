#include "pch.h"
#include "Player.h"

Player::Player()
{
	playerInfo = new Protocol::PlayerInfo();
}

Player::~Player()
{
	delete playerInfo;
}

void Player::OnDamaged(int32 damage)
{
	if (isDead)
		return;

	hp -= damage;

	if (hp <= 0)
	{
		hp = 0;
		isDead = true;
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
	_lastSkillUseTime[skillId] = now;
}

float Player::GetSkillCooldown(int32 skillId) const
{
	switch (skillId)
	{
	case 0: return 0.75f; // Ice
	case 1: return 4.0f; // Fire
	default: return 1.0f;
	}
}