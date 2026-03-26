#pragma once
#include "Mob.h"
#include "JobQueue.h"

enum class RoomType
{
	Hunting,
	Battle
};

class Room : public JobQueue
{
public:
	Room(RoomType type);
	virtual ~Room();

	bool HandleEnterPlayerLocked(PlayerRef player, RoomRef self);
	void HandleClientLevelReady(PlayerRef player);
	bool HandleLeavePlayerLocked(PlayerRef player);

	void HandleMoveLocked(Protocol::C_MOVE& pkt);

public:
	void Init();
	void UpdateTick(float deltaTime);
	void SpawnRandomMobs();
	void Clear(); // 모든 플레이어 제거
	void Broadcast(SendBufferRef sendBuffer, uint64 exceptId = 0);
	void HandleMoveMobLocked(uint64 mobId, float x, float y, float z);
	void HandleAttackMobLocked(uint64 playerId, uint64 mobId);
	void BroadcastUseSkill(uint64 playerId, uint32 skillId);
	void HandlePlayerHit(uint64 attackerId, uint64 targetId);
	void HandleAttackPlayerLocked(uint64 attackerId, uint64 targetId, uint32 skillId);
	RoomType GetRoomType() { return _roomType; }
	bool EnterPlayer(PlayerRef player, RoomRef self);
	void StartReturnToMap1Timer();
	void SendExistingPlayersTo(GameSessionRef session, uint64 excludeObjectId);
	void BroadcastPlayerSpawn(PlayerRef player);

private:
	bool LeavePlayer(uint64 objectId);

private:
	unordered_map<uint64, PlayerRef> _players;
	unordered_map<uint64, MobRef> _mobs;
	bool bReturnToMap1TimerStarted = false;

	float _timer = 0.0f;
	RoomType _roomType;

	USE_LOCK;
};
