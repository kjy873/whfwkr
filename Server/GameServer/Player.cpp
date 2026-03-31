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