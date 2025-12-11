#pragma once
#include <atomic>
#include <memory>

class GameSession;
class Player;

using GameSessionRef = std::shared_ptr<GameSession>;
using PlayerRef = std::shared_ptr<Player>;

class ObjectUtils
{
public:
    static void ResetIdGenerator();
    static uint64 GenerateId();        // 플레이어/몬스터 ID 생성 공통 함수
    static PlayerRef CreatePlayer(GameSessionRef session);

private:
    static std::atomic<uint64> s_idGenerator;
};
