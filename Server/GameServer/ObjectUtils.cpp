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

/*
PlayerRef ObjectUtils::CreatePlayer(GameSessionRef session)
{
    const uint64 newId = GenerateId();

    PlayerRef player = std::make_shared<Player>();
    player->playerInfo->set_object_id(newId);

    player->session = session;
    session->player.store(player);

    return player;
}
*/

static atomic<uint64> g_IdGenerator = 1;

PlayerRef ObjectUtils::CreatePlayer(GameSessionRef session)
{
    // 1. 새로운 플레이어 객체 생성
    PlayerRef player = make_shared<Player>();

    // 2. 고유 ID 부여 (할당 후 1 증가)
    // 이 값은 중복되지 않고 접속하는 순서대로 1, 2, 3... 순서로 부여됩니다.
    uint64 newId = g_IdGenerator.fetch_add(1);
    player->playerInfo->set_object_id(newId);

    // 3. 세션 연결 및 기타 설정
    player->session = session;
    session->player.store(player);

    cout << "[Server] New Player Created. ID: " << newId << endl;

    return player;
}