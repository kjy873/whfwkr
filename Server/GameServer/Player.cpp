#include "pch.h"
#include "Player.h"
#include "Room.h"
#include "RoomManager.h"

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
	if (Hp <= 0)
	{
		cout << "[OnDamaged BLOCK] already dead ObjId="
			<< playerInfo->object_id()
			<< " Hp=" << Hp
			<< endl;
		return;
	}

	int32 PrevHp = Hp;
	Hp -= damage;

	cout << "[OnDamaged] ObjId="
		<< playerInfo->object_id()
		<< " PrevHp=" << PrevHp
		<< " Damage=" << damage
		<< " NewHp=" << Hp
		<< endl;

	if (Hp <= 0)
	{
		Hp = 0;
		playerInfo->set_hp(0);

		DeathCount++;

		if (attacker != nullptr && attacker != shared_from_this())
		{
			attacker->KillCount++;
		}

		RoomRef currentRoom = this->room.lock();
		if (currentRoom)
		{
			currentRoom->BroadcastPlayerStats(shared_from_this());

			if (attacker)
				currentRoom->BroadcastPlayerStats(attacker);
		}

		GRoomManager.UpdatePlayerScore(shared_from_this());

		if (attacker)
			GRoomManager.UpdatePlayerScore(attacker);
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