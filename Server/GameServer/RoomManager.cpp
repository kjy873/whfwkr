#include "pch.h"
#include "Player.h"
#include "GameSession.h"
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

    {
        WRITE_LOCK;

        if (_roundPlayers.empty())
            return;

        for (auto& p : _roundPlayers)
        {
            if (p && p->Hp > 0)
                alivePlayers.push_back(p);
        }

        if (alivePlayers.size() <= 1)
        {
            cout << "[RoundEnd] winner decided. aliveCount=" << alivePlayers.size() << endl;
            _roundPlayers.clear();
            _roundTimer = 0.0f;
            _isBattlePhase = false;
            return;
        }

        _roundTimer += deltaTime;

        if (_roundTimer < 60.0f)
            return;

        _roundTimer = 0.0f;

        if (_isBattlePhase == false)
        {
            _isBattlePhase = true;
        }
        else
        {
            _isBattlePhase = false;
        }
    }

    if (_isBattlePhase)
    {
        cout << "[RoundPhase] Hunting -> Battle" << endl;
        MoveRoundPlayersToBattle();
    }
    else
    {
        cout << "[RoundPhase] Battle -> Hunting" << endl;
        MoveRoundPlayersToHunting();
    }
}

void RoomManager::StartRound(const vector<PlayerRef>& players)
{
    WRITE_LOCK;
    _roundPlayers = players;
    _roundTimer = 0.0f;
    _isBattlePhase = false;

    cout << "[StartRound] playerCount=" << _roundPlayers.size()
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
            if (p && p->Hp > 0)
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
            if (p && p->Hp > 0)
                playersToMove.push_back(p);
        }
    }

    for (auto& p : playersToMove)
        MoveToHuntingRoom(p);
}

void RoomManager::MoveToDemoLevel(uint32 targetLevel)
{
    {
        WRITE_LOCK;

        if (_roundPlayers.empty())
        {
            return;
        }

        _roundTimer = 0.0f;

        if (targetLevel == 1)
        {
            _isBattlePhase = false; //PVE
        }
        else if (targetLevel == 2)
        {
            _isBattlePhase = true; //PVP
        }
        else
        {
            return;
        }
    }

    if (targetLevel == 1)
    {
        cout << "[Demo] Move to PVE" << endl;
        MoveRoundPlayersToHunting();
    }
    else if (targetLevel == 2)
    {
        cout << "[Demo] Move to PVP" << endl;
        MoveRoundPlayersToBattle();
    }
}

void RoomManager::MoveToBattleRoom(PlayerRef player)
{
    auto session = player->session.lock();
    if (!session) return;

    RoomRef battleRoom = GetOrCreateBattleRoom();

    battleRoom->DoAsync([battleRoom, player, session]()
        {
            battleRoom->SetRoomType(RoomType::Battle);
            //battleRoom->ApplySpawnByRoomType(player);
            battleRoom->HandleEnterPlayerLocked(player, battleRoom);

            Protocol::S_CHANGE_LEVEL pkt;
            pkt.set_level_name("NewMap");

            auto sendBuffer = ServerPacketHandler::MakeSendBuffer(pkt);
            session->Send(sendBuffer);
        });
}

void RoomManager::MoveToHuntingRoom(PlayerRef player)
{
    auto session = player->session.lock();
    if (!session)
        return;

    RoomRef oldRoom = player->room.lock();

    RoomRef huntingRoom = make_shared<Room>(RoomType::Hunting);
    huntingRoom->Init();

    _rooms.push_back(huntingRoom);

    if (oldRoom)
    {
        oldRoom->DoAsync([oldRoom, huntingRoom, player, session]()
            {
                oldRoom->HandleLeavePlayerLocked(player);

                huntingRoom->DoAsync([huntingRoom, player, session]()
                    {
                        huntingRoom->HandleEnterPlayerLocked(player, huntingRoom);

                        Protocol::S_CHANGE_LEVEL pkt;
                        pkt.set_level_name("LandscapeMap");

                        auto sendBuffer = ServerPacketHandler::MakeSendBuffer(pkt);
                        session->Send(sendBuffer);
                    });
            });
    }
    else
    {
        huntingRoom->DoAsync([huntingRoom, player, session]()
            {
                huntingRoom->HandleEnterPlayerLocked(player, huntingRoom);

                Protocol::S_CHANGE_LEVEL pkt;
                pkt.set_level_name("LandscapeMap");

                auto sendBuffer = ServerPacketHandler::MakeSendBuffer(pkt);
                session->Send(sendBuffer);
            });
    }
}