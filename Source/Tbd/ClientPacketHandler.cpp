#include "ClientPacketHandler.h"
#include "BufferReader.h"
#include "Tbd.h"
#include "MainGameInstance.h"

PacketHandlerFunc GPacketHandler[UINT16_MAX];

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
	if (auto* GameInstance  = Cast<UMainGameInstance>(GWorld->GetGameInstance()))
	{
		GameInstance->HandleSpawn(pkt);
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Entered Game"));
	}

	return true;
}

bool Handle_S_LEAVE_GAME(PacketSessionRef& session, Protocol::S_LEAVE_GAME& pkt)
{
	if ( auto* GameInstance = Cast<UMainGameInstance>(GWorld->GetGameInstance()))
	{

	}
	return false;
}

bool Handle_S_SPAWN(PacketSessionRef& session, Protocol::S_SPAWN& pkt)
{
	if (auto* GameInstance = Cast<UMainGameInstance>(GWorld->GetGameInstance()))
	{
		GameInstance->HandleSpawn(pkt);
	}
	return false;
}

bool Handle_S_DESPAWN(PacketSessionRef& session, Protocol::S_DESPAWN& pkt)
{
	if (auto* GameInstance = Cast<UMainGameInstance>(GWorld->GetGameInstance()))
	{
		GameInstance->HandleDespawn(pkt);
	}
	return false;
}

bool Handle_S_MOVE(PacketSessionRef& session, Protocol::S_MOVE& pkt)
{
	if (auto* GameInstance = Cast<UMainGameInstance>(GWorld->GetGameInstance()))
	{
		GameInstance->HandleMove(pkt);
	}
	return true;
}

bool Handle_S_CHAT(PacketSessionRef& session, Protocol::S_CHAT& pkt)
{
	auto Msg = pkt.msg();

	return true;
}

bool Handle_S_SPAWN_MOB(PacketSessionRef& session, Protocol::S_SPAWN_MOB& pkt)
{
	UE_LOG(LogTemp, Warning, TEXT("[Handle_S_SPAWN_MOB] ===== PACKET RECEIVED ====="));
	UE_LOG(LogTemp, Warning, TEXT("[Handle_S_SPAWN_MOB] Received %d mobs"), pkt.mobs_size());

	if (GWorld == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[Handle_S_SPAWN_MOB] GWorld is nullptr!"));
		return false;
	}

	auto* GameInstance = Cast<UMainGameInstance>(GWorld->GetGameInstance());
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
	if (auto* GameInstance = Cast<UMainGameInstance>(GWorld->GetGameInstance()))
	{
		GameInstance->HandleDespawnMob(pkt);
	}

	return true;
}

bool Handle_S_MOVE_MOB(PacketSessionRef& session, Protocol::S_MOVE_MOB& pkt)
{
	if (auto* GameInstance = Cast<UMainGameInstance>(GWorld->GetGameInstance()))
	{
		GameInstance->HandleMoveMob(pkt);
	}

	return false;
}

bool Handle_S_DAMAGE_MOB(PacketSessionRef& session, Protocol::S_DAMAGE_MOB& pkt)
{
	if (auto* GameInstance = Cast<UMainGameInstance>(GWorld->GetGameInstance()))
	{
		GameInstance->HandleDamageMob(pkt);
	}
	return false;
}