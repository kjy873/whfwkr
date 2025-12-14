#pragma once
#include "Mob.h"
#include "JobQueue.h"

class Room : public JobQueue
{
public:
	Room();
	virtual ~Room();

	bool HandleEnterPlayerLocked(PlayerRef player);
	bool HandleLeavePlayerLocked(PlayerRef player);

	void HandleMoveLocked(Protocol::C_MOVE& pkt);

public:
	void Init();
	void UpdateTick();
	void SpawnRandomMobs();
	void Clear(); // 모든 플레이어 제거
	void Broadcast(SendBufferRef sendBuffer, uint64 exceptId = 0);
	void HandleMoveMobLocked(uint64 mobId, float x, float y, float z);
	void HandleAttackMobLocked(uint64 playerId, uint64 mobId);
	void BroadcastUseSkill(uint64 playerId, uint32 skillId);

private:
	bool EnterPlayer(PlayerRef player);
	bool LeavePlayer(uint64 objectId);

	void UpdateMobs();

private:
	unordered_map<uint64, PlayerRef> _players;
	unordered_map<uint64, MobRef> _mobs;

	USE_LOCK;
};

extern RoomRef GRoom;