#include "pch.h"
#include "ObjectUtils.h"
#include "Player.h"
#include "GameSession.h"

std::atomic<uint64> ObjectUtils::s_idGenerator = 1;

void ObjectUtils::ResetIdGenerator()
{
    s_idGenerator.store(1);
}

uint64 ObjectUtils::GenerateId()
{
    return s_idGenerator.fetch_add(1);
}

PlayerRef ObjectUtils::CreatePlayer(GameSessionRef session)
{
    const uint64 newId = GenerateId();

    PlayerRef player = std::make_shared<Player>();
    player->playerInfo->set_object_id(newId);

    player->session = session;
    session->player.store(player);

    return player;
}
