#include "ClientPacketHandler.h"
#include "BufferReader.h"
#include "Tbd.h"
#include "MainGameInstance.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

PacketHandlerFunc GPacketHandler[UINT16_MAX];

static UMainGameInstance* GetMainGameInstance()
{
	UWorld* World = GWorld;
	if (!World && GEngine)
		World = GEngine->GetCurrentPlayWorld();
	return World ? Cast<UMainGameInstance>(World->GetGameInstance()) : nullptr;
}

bool Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	return false;
}

bool Handle_S_LOGIN(PacketSessionRef& session, Protocol::S_LOGIN& pkt)
{
	for(auto& Player : pkt.players())
	{
	}
	for (int32 i = 0; i < pkt.players_size(); i++)
	{
		const Protocol::PlayerInfo& Player = pkt.players(i);
	}

	Protocol::C_ENTER_GAME EnterGamePkt;
	EnterGamePkt.set_playerindex(0);
	SEND_PACKET(EnterGamePkt);

	return true;
}

bool Handle_S_ENTER_GAME(PacketSessionRef& session, Protocol::S_ENTER_GAME& pkt)
{
	if (auto* GameInstance = GetMainGameInstance())
	{
		GameInstance->HandleSpawn(pkt);
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Entered Game"));
	}
	return true;
}

bool Handle_S_LEAVE_GAME(PacketSessionRef& session, Protocol::S_LEAVE_GAME& pkt)
{
	if (auto* GameInstance = GetMainGameInstance()) { }
	return false;
}

bool Handle_S_SPAWN(PacketSessionRef& session, Protocol::S_SPAWN& pkt)
{
	if (auto* GameInstance = GetMainGameInstance())
		GameInstance->HandleSpawn(pkt);
	return false;
}

bool Handle_S_DESPAWN(PacketSessionRef& session, Protocol::S_DESPAWN& pkt)
{
	if (auto* GameInstance = GetMainGameInstance())
		GameInstance->HandleDespawn(pkt);
	return false;
}

bool Handle_S_MOVE(PacketSessionRef& session, Protocol::S_MOVE& pkt)
{
	if (auto* GameInstance = GetMainGameInstance())
		GameInstance->HandleMove(pkt);
	return true;
}

bool Handle_S_CHAT(PacketSessionRef& session, Protocol::S_CHAT& pkt)
{
	auto Msg = pkt.msg();

	return true;
}

bool Handle_S_DAMAGE_PLAYER(PacketSessionRef& session, Protocol::S_DAMAGE_PLAYER& pkt)
{
	return false;
}

bool Handle_S_PLAYER_DEAD(PacketSessionRef& session, Protocol::S_PLAYER_DEAD& pkt)
{
	return false;
}

bool Handle_S_SPAWN_MOB(PacketSessionRef& session, Protocol::S_SPAWN_MOB& pkt)
{
	UE_LOG(LogTemp, Warning, TEXT("[Handle_S_SPAWN_MOB] ===== PACKET RECEIVED ====="));
	UE_LOG(LogTemp, Warning, TEXT("[Handle_S_SPAWN_MOB] Received %d mobs"), pkt.mobs_size());

	auto* GameInstance = GetMainGameInstance();
	if (GameInstance == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[Handle_S_SPAWN_MOB] GameInstance is nullptr!"));
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("[Handle_S_SPAWN_MOB] Calling HandleSpawnMob..."));
		GameInstance->HandleSpawnMob(pkt);
	UE_LOG(LogTemp, Warning, TEXT("[Handle_S_SPAWN_MOB] HandleSpawnMob returned"));

	return false;
}

bool Handle_S_DESPAWN_MOB(PacketSessionRef& session, Protocol::S_DESPAWN_MOB& pkt)
{
	if (auto* GameInstance = GetMainGameInstance())
		GameInstance->HandleDespawnMob(pkt);
	return true;
}

bool Handle_S_MOVE_MOB(PacketSessionRef& session, Protocol::S_MOVE_MOB& pkt)
{
	if (auto* GameInstance = GetMainGameInstance())
		GameInstance->HandleMoveMob(pkt);
	return false;
}

bool Handle_S_DAMAGE_MOB(PacketSessionRef& session, Protocol::S_DAMAGE_MOB& pkt)
{
	if (auto* GameInstance = GetMainGameInstance())
		GameInstance->HandleDamageMob(pkt);
	return false;
}

bool Handle_S_USE_SKILL(PacketSessionRef& session, Protocol::S_USE_SKILL& pkt)
{
	int32 playerId = static_cast<int32>(pkt.playerid());
	int32 skillId = static_cast<int32>(pkt.skillid());

	if (auto* GI = GetMainGameInstance())
		GI->OnRecvUseSkill(pkt);
	return true;
}

bool Handle_S_PROJECTILE_HIT(PacketSessionRef& session, Protocol::S_PROJECTILE_HIT& pkt)
{
	if (auto* GI = GetMainGameInstance())
		GI->OnRecvProjectileHit(pkt);
	return true;
}

bool Handle_S_PROJECTILE_DESTROY(PacketSessionRef& session, Protocol::S_PROJECTILE_DESTROY& pkt)
{
	if (auto* GI = GetMainGameInstance())
		GI->OnRecvProjectileDestroy(pkt);
	return true;
}
