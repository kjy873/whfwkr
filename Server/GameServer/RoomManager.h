#pragma once

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

    RoomRef GetBattleRoom() { return _battleRoom; }

    static RoomRef _battleRoom;
    static RoomRef _lobbyRoom;

private:
    USE_LOCK;
    vector<RoomRef> _rooms;
    uint64 _roomIdGenerator = 1;

    vector<PlayerRef> _roundPlayers;
    float _roundTimer = 0.0f;
    bool _isBattlePhase = false;
};

extern RoomManager GRoomManager;
