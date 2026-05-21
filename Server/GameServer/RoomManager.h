#pragma once

struct PlayerScoreData
{
    int32 KillCount = 0;
    int32 DeathCount = 0;
    int32 MonsterKillCount = 0;
};

class RoomManager
{
public:
    RoomManager();

    RoomRef GetOrCreateBattleRoom();
    RoomRef GetOrCreateLobbyRoom();

    void MoveToBattleRoom(PlayerRef player);
    void MoveToHuntingRoom(PlayerRef player);
    void Update(float deltaTime);

    void StartRound(const vector<PlayerRef>& players);
    void MoveRoundPlayersToBattle();
    void MoveRoundPlayersToHunting();
    void MoveToDemoLevel(uint32 targetLevel);

    void RespawnRoundPlayer(PlayerRef player);
    void ResetPlayerForPhase(PlayerRef player, bool bMoveToPVE);

    RoomRef GetBattleRoom() { return _battleRoom; }

    static RoomRef _battleRoom;
    static RoomRef _lobbyRoom;

    void HandleLevelReady(PlayerRef player);
    void ClearPendingMoveForPlayer(uint64 objectId);

private:
    unordered_map<uint64, RoomRef> _pendingMoveRoom;

public:
    void UpdatePlayerScore(PlayerRef player);
    void BroadcastGameResult();
    void ClearScoreMap();

private:
    std::unordered_map<uint64, PlayerScoreData> _scoreMap;

private:
    USE_LOCK;
    vector<RoomRef> _rooms;
    uint64 _roomIdGenerator = 1;

    int32 _currentRound = 0;
    int32 _maxRound = 2;
    bool _gameFinished = false;

    vector<PlayerRef> _roundPlayers;
    float _roundTimer = 0.0f;
    bool _isBattlePhase = false;
};

extern RoomManager GRoomManager;
