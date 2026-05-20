#include "pch.h"
#include "Player.h"
#include "GameSession.h"
#include "GameSessionManager.h"
#include "ServerPacketHandler.h"
#include "Room.h"
#include "RoomManager.h"

RoomManager GRoomManager;
RoomRef RoomManager::_battleRoom = nullptr;
RoomRef RoomManager::_lobbyRoom = nullptr;

RoomManager::RoomManager()
{
    cout << "RoomManager Constructed: " << this << endl;
}

RoomRef RoomManager::GetOrCreateBattleRoom()
{
    WRITE_LOCK;

    if (_battleRoom == nullptr)
    {
        _battleRoom = make_shared<Room>(RoomType::Battle);
        _battleRoom->Init();

        _rooms.push_back(_battleRoom);
    }

    return _battleRoom;
}

RoomRef RoomManager::GetOrCreateLobbyRoom()
{
    WRITE_LOCK;

    if (_lobbyRoom == nullptr)
    {
        _lobbyRoom = make_shared<Room>(RoomType::Lobby);
        _lobbyRoom->Init();

        _rooms.push_back(_lobbyRoom);
    }

    return _lobbyRoom;
}

void RoomManager::Update(float deltaTime)
{
    vector<PlayerRef> alivePlayers;

    bool shouldMoveToBattle = false;
    bool shouldMoveToHunting = false;
    bool shouldBroadcastGameResult = false;

    {
        WRITE_LOCK;

        if (_gameFinished)
            return;

        if (_roundPlayers.empty())
            return;

        for (auto& p : _roundPlayers)
        {
            if (p && p->Hp > 0)
                alivePlayers.push_back(p);
        }

        _roundTimer += deltaTime;

        bool bPhaseEnded = false;

        if (_roundTimer >= 60.0f)
        {
            cout << "[Phase Timer End] isBattlePhase=" << _isBattlePhase << endl;
            bPhaseEnded = true;
        }

        if (bPhaseEnded == false)
            return;

        _roundTimer = 0.0f;

        if (_isBattlePhase == false)
        {
            _isBattlePhase = true;
            shouldMoveToBattle = true;

            cout << "[RoundPhase] Hunting -> Battle"
                << " currentRound=" << _currentRound
                << endl;
        }
        else
        {
            _currentRound++;

            cout << "[Round End] currentRound=" << _currentRound
                << " maxRound=" << _maxRound
                << endl;

            if (_currentRound >= _maxRound)
            {
                _gameFinished = true;
                shouldBroadcastGameResult = true;

                cout << "[Game Finished] BroadcastGameResult" << endl;
            }
            else
            {
                _isBattlePhase = false;
                shouldMoveToHunting = true;

                cout << "[RoundPhase] Battle -> Hunting"
                    << " nextRound=" << (_currentRound + 1)
                    << endl;
            }
        }
    }

    if (shouldBroadcastGameResult)
    {
        BroadcastGameResult();
        return;
    }

    if (shouldMoveToBattle)
    {
        MoveRoundPlayersToBattle();
    }
    else if (shouldMoveToHunting)
    {
        MoveRoundPlayersToHunting();
    }
}

void RoomManager::StartRound(const vector<PlayerRef>& players)
{
    WRITE_LOCK;

    _roundPlayers = players;
    _roundTimer = 0.0f;
    _isBattlePhase = false;

    _currentRound = 0;
    _maxRound = 2;
    _gameFinished = false;

    _scoreMap.clear();

    cout << "[StartRound] playerCount=" << _roundPlayers.size()
        << " currentRound=" << _currentRound
        << " maxRound=" << _maxRound
        << " tick=" << GetTickCount64()
        << endl;
}

void RoomManager::MoveRoundPlayersToBattle()
{
    vector<PlayerRef> playersToMove;

    {
        WRITE_LOCK;
        for (auto& p : _roundPlayers)
        {
            if (p)
                playersToMove.push_back(p);
        }
    }

    for (auto& p : playersToMove)
        MoveToBattleRoom(p);
}

void RoomManager::MoveRoundPlayersToHunting()
{
    vector<PlayerRef> playersToMove;

    {
        WRITE_LOCK;
        for (auto& p : _roundPlayers)
        {
            if (p)
                playersToMove.push_back(p);
        }
    }

    for (auto& p : playersToMove)
        MoveToHuntingRoom(p);
}

void RoomManager::MoveToDemoLevel(uint32 targetLevel)
{
    bool shouldMoveToPVE = false;
    bool shouldMoveToPVP = false;
    bool shouldBroadcastGameResult = false;

    {
        WRITE_LOCK;

        if (!_pendingMoveRoom.empty())
        {
            cout << "[MoveToDemoLevel BLOCK] already waiting level ready. pending="
                << _pendingMoveRoom.size()
                << " targetLevel=" << targetLevel
                << endl;

            return;
        }

        if (_gameFinished)
            return;

        if (_roundPlayers.empty())
            return;

        _roundTimer = 0.0f;

        if (targetLevel == 1)
        {
            // PVP -> PVE 로 넘어가는 순간 = PVP 종료 = 1라운드 완료
            if (_isBattlePhase == true)
            {
                _currentRound++;

                cout << "[Demo Round End] currentRound=" << _currentRound
                    << " maxRound=" << _maxRound
                    << endl;

                if (_currentRound >= _maxRound)
                {
                    _gameFinished = true;
                    shouldBroadcastGameResult = true;

                    cout << "[Demo Game Finished] BroadcastGameResult" << endl;
                }
                else
                {
                    _isBattlePhase = false;
                    shouldMoveToPVE = true;
                }
            }
            else
            {
                _isBattlePhase = false;
                shouldMoveToPVE = true;
            }
        }
        else if (targetLevel == 2)
        {
            _isBattlePhase = true;
            shouldMoveToPVP = true;
        }
        else
        {
            return;
        }
    }

    if (shouldBroadcastGameResult)
    {
        BroadcastGameResult();
        return;
    }

    if (shouldMoveToPVE)
    {
        cout << "[Demo] Move to PVE" << endl;
        MoveRoundPlayersToHunting();
    }
    else if (shouldMoveToPVP)
    {
        cout << "[Demo] Move to PVP" << endl;
        MoveRoundPlayersToBattle();
    }
}

void RoomManager::RespawnRoundPlayer(PlayerRef player)
{
    if (player == nullptr || player->playerInfo == nullptr)
        return;

    player->Hp = 50;

    RoomRef currentRoom = player->room.lock();
    if (currentRoom)
    {
        currentRoom->BroadcastPlayerStats(player);
    }

    cout << "[RespawnRoundPlayer] ObjId="
        << player->playerInfo->object_id()
        << " Hp=" << player->Hp
        << endl;
}

void RoomManager::ResetPlayerForPhase(PlayerRef player, bool bMoveToPVE)
{
    if (player == nullptr || player->playerInfo == nullptr)
        return;

    if (bMoveToPVE)
    {
        player->Hp = 100;
    }
    else
    {
        player->Hp = 50;
    }

    cout << "[ResetPlayerForPhase] ObjId="
        << player->playerInfo->object_id()
        << " Hp=" << player->Hp
        << " MoveToPVE=" << bMoveToPVE
        << endl;
}

void RoomManager::HandleLevelReady(PlayerRef player)
{
    if (player == nullptr || player->playerInfo == nullptr)
        return;

    const uint64 objectId = player->playerInfo->object_id();

    RoomRef targetRoom = nullptr;

    {
        WRITE_LOCK;

        auto it = _pendingMoveRoom.find(objectId);
        if (it != _pendingMoveRoom.end())
        {
            targetRoom = it->second;
            _pendingMoveRoom.erase(it);

            cout << "[HandleLevelReady] Found pending room ObjId="
                << objectId
                << " TargetRoom=" << static_cast<int32>(targetRoom->GetRoomType())
                << endl;
        }
    }

    if (targetRoom == nullptr)
    {
        RoomRef curRoom = player->room.lock();

        cout << "[HandleLevelReady BLOCK] pending room not found ObjId="
            << objectId
            << " CurrentRoom=" << (curRoom ? static_cast<int32>(curRoom->GetRoomType()) : -1)
            << " Pos=("
            << player->playerInfo->x() << ", "
            << player->playerInfo->y() << ", "
            << player->playerInfo->z() << ")"
            << endl;

        return;
    }

    targetRoom->DoAsync([targetRoom, player]()
        {
            targetRoom->HandleClientLevelReady(player);
        });
}

void RoomManager::UpdatePlayerScore(PlayerRef player)
{
    if (player == nullptr || player->playerInfo == nullptr)
        return;

    uint64 objectId = player->playerInfo->object_id();

    _scoreMap[objectId].KillCount = player->KillCount;
    _scoreMap[objectId].DeathCount = player->DeathCount;
    _scoreMap[objectId].MonsterKillCount = player->MonsterKillCount;
}

void RoomManager::BroadcastGameResult()
{
    int32 bestScore = INT_MIN;
    uint64 winnerObjectId = 0;

    if (_scoreMap.empty())
    {
        cout << "[GameResult] No valid score data" << endl;
        return;
    }

    for (auto& pair : _scoreMap)
    {
        uint64 objectId = pair.first;
        PlayerScoreData& stats = pair.second;

        int32 score =
            stats.KillCount * 5 +
            stats.MonsterKillCount * 2 -
            stats.DeathCount;

        if (score > bestScore)
        {
            bestScore = score;
            winnerObjectId = objectId;
        }
    }

    if (winnerObjectId == 0)
    {
        cout << "[GameResult] No winner found" << endl;
        return;
    }

    Protocol::S_GAME_RESULT pkt;

    for (auto& pair : _scoreMap)
    {
        uint64 objectId = pair.first;
        PlayerScoreData& stats = pair.second;

        int32 score =
            stats.KillCount * 5 +
            stats.MonsterKillCount * 2 -
            stats.DeathCount;

        Protocol::GameResultInfo* result = pkt.add_results();

        result->set_object_id(objectId);
        result->set_kill_count(stats.KillCount);
        result->set_death_count(stats.DeathCount);
        result->set_monster_kill_count(stats.MonsterKillCount);
        result->set_score(score);
        result->set_is_winner(objectId == winnerObjectId);

        cout << "[GameResult Add] ObjId=" << objectId
            << " Score=" << score
            << " IsWinner=" << (objectId == winnerObjectId)
            << endl;
    }

    cout << "[GameResult Winner] ObjId=" << winnerObjectId
        << " BestScore=" << bestScore
        << endl;

    SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(pkt);
    GSessionManager.Broadcast(sendBuffer);
}

void RoomManager::ClearScoreMap()
{
    _scoreMap.clear();
    cout << "[RoomManager ScoreMap Clear]" << endl;
}

void RoomManager::MoveToBattleRoom(PlayerRef player)
{
    if (player == nullptr || player->playerInfo == nullptr)
        return;

    auto session = player->session.lock();
    if (!session)
        return;

    RoomRef oldRoom = player->room.lock();

    RoomRef battleRoom = GetOrCreateBattleRoom();

    {
        WRITE_LOCK;

        _pendingMoveRoom[player->playerInfo->object_id()] = battleRoom;

        cout << "[PendingMoveRoom SET] ObjId="
            << player->playerInfo->object_id()
            << " TargetRoom=Battle"
            << endl;
    }

    if (oldRoom)
    {
        oldRoom->DoAsync([oldRoom, battleRoom, player, session]()
            {
                // 레벨 이동 중에는 S_LEAVE_GAME / S_DESPAWN 보내지 말고
                // 이전 방 목록에서만 조용히 제거
                oldRoom->RemovePlayerOnly(player->playerInfo->object_id());

                battleRoom->DoAsync([battleRoom, player, session]()
                    {
                        battleRoom->SetRoomType(RoomType::Battle);

                        if (player->Hp <= 0)
                        {
                            player->Hp = 50;
                            player->playerInfo->set_hp(50);

                            cout << "[Battle Respawn] ObjId="
                                << player->playerInfo->object_id()
                                << " Hp=50"
                                << endl;
                        }
                        else
                        {
                            cout << "[Battle Enter Keep Hp] ObjId="
                                << player->playerInfo->object_id()
                                << " Hp=" << player->Hp
                                << endl;
                        }

                        Protocol::S_CHANGE_LEVEL pkt;
                        pkt.set_level_name("/Game/Level/NewMap");

                        auto sendBuffer = ServerPacketHandler::MakeSendBuffer(pkt);
                        session->Send(sendBuffer);

                        cout << "[MoveToBattleRoom] Send S_CHANGE_LEVEL ObjId="
                            << player->playerInfo->object_id()
                            << endl;
                    });
            });
    }
    else
    {
        battleRoom->DoAsync([battleRoom, player, session]()
            {
                battleRoom->SetRoomType(RoomType::Battle);

                if (player->Hp <= 0)
                {
                    player->Hp = 50;
                    player->playerInfo->set_hp(50);

                    cout << "[Battle Respawn] ObjId="
                        << player->playerInfo->object_id()
                        << " Hp=50"
                        << endl;
                }
                else
                {
                    cout << "[Battle Enter Keep Hp] ObjId="
                        << player->playerInfo->object_id()
                        << " Hp=" << player->Hp
                        << endl;
                }

                Protocol::S_CHANGE_LEVEL pkt;
                pkt.set_level_name("/Game/Level/NewMap");

                auto sendBuffer = ServerPacketHandler::MakeSendBuffer(pkt);
                session->Send(sendBuffer);

                cout << "[MoveToBattleRoom] Send S_CHANGE_LEVEL ObjId="
                    << player->playerInfo->object_id()
                    << endl;
            });
    }
}

void RoomManager::MoveToHuntingRoom(PlayerRef player)
{
    if (player == nullptr || player->playerInfo == nullptr)
        return;

    auto session = player->session.lock();
    if (!session)
        return;

    ResetPlayerForPhase(player, true);

    RoomRef oldRoom = player->room.lock();

    RoomRef huntingRoom = make_shared<Room>(RoomType::Hunting);
    huntingRoom->Init();

    {
        WRITE_LOCK;

        _rooms.push_back(huntingRoom);

        _pendingMoveRoom[player->playerInfo->object_id()] = huntingRoom;

        cout << "[PendingMoveRoom SET] ObjId="
            << player->playerInfo->object_id()
            << " TargetRoom=Hunting"
            << endl;
    }

    if (oldRoom)
    {
        oldRoom->DoAsync([oldRoom, huntingRoom, player, session]()
            {
                // 레벨 이동 중에는 S_LEAVE_GAME / S_DESPAWN 보내면 안 됨
                // 서버 방 목록에서만 조용히 제거
                oldRoom->RemovePlayerOnly(player->playerInfo->object_id());

                huntingRoom->DoAsync([huntingRoom, player, session]()
                    {
                        Protocol::S_CHANGE_LEVEL pkt;
                        pkt.set_level_name("/Game/Level/LandscapeMap");

                        auto sendBuffer = ServerPacketHandler::MakeSendBuffer(pkt);
                        session->Send(sendBuffer);

                        cout << "[MoveToHuntingRoom] Send S_CHANGE_LEVEL ObjId="
                            << player->playerInfo->object_id()
                            << endl;
                    });
            });
    }
    else
    {
        huntingRoom->DoAsync([huntingRoom, player, session]()
            {
                Protocol::S_CHANGE_LEVEL pkt;
                pkt.set_level_name("/Game/Level/LandscapeMap");

                auto sendBuffer = ServerPacketHandler::MakeSendBuffer(pkt);
                session->Send(sendBuffer);

                cout << "[MoveToHuntingRoom] Send S_CHANGE_LEVEL ObjId="
                    << player->playerInfo->object_id()
                    << endl;
            });
    }
}