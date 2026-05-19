#include "ClientPacketHandler.h"
#include "BufferReader.h"
#include "Tbd.h"
#include "MainGameInstance.h"
#include "PacketSession.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Player/PlayerCharacter.h"

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
	UE_LOG(LogTemp, Warning, TEXT("[Handle_S_LOGIN] players_size=%d"), pkt.players_size());

	if (pkt.players_size() <= 0)
	{
		UE_LOG(LogTemp, Error, TEXT("[Handle_S_LOGIN] No players in packet"));
		return false;
	}

	for (int32 i = 0; i < pkt.players_size(); i++)
	{
		const Protocol::PlayerInfo& Player = pkt.players(i);

		UE_LOG(LogTemp, Warning, TEXT("[Handle_S_LOGIN] idx=%d objectId=%lld"),
			i,
			static_cast<long long>(Player.object_id()));
	}

	UE_LOG(LogTemp, Warning, TEXT("[Handle_S_LOGIN] Login success. Wait for Start button."));

	return true;
}

bool Handle_S_ENTER_GAME(PacketSessionRef& session, Protocol::S_ENTER_GAME& pkt)
{
	Protocol::PlayerInfo PlayerCopy = pkt.player();

	if (auto* GI = GetMainGameInstance())
	{
		if (GI->bChangingLevel)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Handle_S_ENTER_GAME] Skip: changing level"));
			return true;
		}

		AsyncTask(ENamedThreads::GameThread, [GI, PlayerCopy]()
			{
				if (GI == nullptr || GI->bChangingLevel)
					return;

				GI->HandleSpawn(PlayerCopy, true);
			});
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
	TArray<Protocol::PlayerInfo> PlayerCopies;
	PlayerCopies.Reserve(pkt.players_size());

	for (const auto& Info : pkt.players())
		PlayerCopies.Add(Info);

	if (auto* GI = GetMainGameInstance())
	{
		if (GI->bChangingLevel)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Handle_S_SPAWN] Skip: changing level"));
			return true;
		}

		AsyncTask(ENamedThreads::GameThread, [GI, PlayerCopies]()
			{
				if (GI == nullptr || GI->bChangingLevel)
					return;

				for (const auto& Info : PlayerCopies)
				{
					if (Info.object_id() == GI->MyObjectId)
						continue;

					GI->HandleSpawn(Info, false);
				}
			});
	}

	return true;
}

bool Handle_S_DESPAWN(PacketSessionRef& session, Protocol::S_DESPAWN& pkt)
{
	if (auto* GameInstance = GetMainGameInstance())
		GameInstance->HandleDespawn(pkt);
	return false;
}

bool Handle_S_MOVE(PacketSessionRef& session, Protocol::S_MOVE& pkt)
{
	UE_LOG(LogTemp, Warning, TEXT("[Handle_S_MOVE] packet received objId=%llu"),
		static_cast<unsigned long long>(pkt.info().object_id()));

	if (auto* GameInstance = GetMainGameInstance())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Handle_S_MOVE] GameInstance valid"));
		GameInstance->HandleMove(pkt);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[Handle_S_MOVE] GameInstance nullptr"));
	}

	return true;
}

bool Handle_S_CHAT(PacketSessionRef& session, Protocol::S_CHAT& pkt)
{
	auto Msg = pkt.msg();

	return true;
}

bool Handle_S_PLAYER_STATS(PacketSessionRef& session, Protocol::S_PLAYER_STATS& pkt)
{
	UE_LOG(LogTemp, Warning, TEXT("[Handle_S_PLAYER_STATS] ObjId=%llu K=%d D=%d M=%d"),
		(unsigned long long)pkt.object_id(),
		pkt.kill_count(),
		pkt.death_count(),
		pkt.monster_kill_count());

	if (GWorld == nullptr)
		return false;

	UMainGameInstance* GI = Cast<UMainGameInstance>(GWorld->GetGameInstance());
	if (GI)
	{
		GI->HandlePlayerStats(pkt);
	}

	return true;
}

bool Handle_S_GAME_RESULT(PacketSessionRef& session, Protocol::S_GAME_RESULT& pkt)
{
	UE_LOG(LogTemp, Warning, TEXT("[Handle_S_GAME_RESULT] ENTER ResultCount=%d"), pkt.results_size());

	for (int32 i = 0; i < pkt.results_size(); i++)
	{
		const Protocol::GameResultInfo& Result = pkt.results(i);

		UE_LOG(LogTemp, Warning,
			TEXT("[Handle_S_GAME_RESULT] ObjId=%llu K=%d D=%d M=%d Score=%d Winner=%d"),
			Result.object_id(),
			Result.kill_count(),
			Result.death_count(),
			Result.monster_kill_count(),
			Result.score(),
			Result.is_winner() ? 1 : 0
		);
	}

	if (session == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[Handle_S_GAME_RESULT] session nullptr"));
		return true;
	}

	UMainGameInstance* GameInstance = session->GetOwnerGameInstance();
	if (GameInstance == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[Handle_S_GAME_RESULT] OwnerGameInstance nullptr"));
		return true;
	}

	UE_LOG(LogTemp, Warning, TEXT("[Handle_S_GAME_RESULT] Call HandleGameResult MyObjectId=%llu"),
		GameInstance->MyObjectId);

	GameInstance->HandleGameResult(pkt);

	return true;
}

bool Handle_S_DAMAGE_PLAYER(PacketSessionRef& session, Protocol::S_DAMAGE_PLAYER& pkt)
{
	UE_LOG(LogTemp, Warning, TEXT("[Handle_S_DAMAGE_PLAYER] packet arrived session=%p obj=%llu damage=%d"),
		session.Get(),
		(unsigned long long)pkt.object_id(),
		(int32)pkt.damage());

	if (session == nullptr)
		return false;

	UMainGameInstance* GI = session->GetOwnerGameInstance();
	if (GI == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Handle_S_DAMAGE_PLAYER] OwnerGameInstance is null session=%p"),
			session.Get());
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("[Handle_S_DAMAGE_PLAYER] dispatch only owner GI=%p"),
		GI);

	GI->HandleDamage(pkt);
	return true;
}

bool Handle_S_PLAYER_DEAD(PacketSessionRef& session, Protocol::S_PLAYER_DEAD& pkt)
{
	UE_LOG(LogTemp, Warning, TEXT("[Handle_S_PLAYER_DEAD] packet arrived"));

	if (GEngine == nullptr)
		return false;

	bool bHandled = false;

	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		if (Context.OwningGameInstance == nullptr)
			continue;

		UMainGameInstance* GI = Cast<UMainGameInstance>(Context.OwningGameInstance);
		if (GI == nullptr)
			continue;

		UE_LOG(LogTemp, Warning, TEXT("[Handle_S_PLAYER_DEAD] dispatch GI=%p WorldType=%d"),
			GI, (int32)Context.WorldType);

		GI->HandleDie(pkt);
		bHandled = true;
	}

	return bHandled;
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
	if (GWorld == nullptr)
		return false;

	UMainGameInstance* GI = Cast<UMainGameInstance>(GWorld->GetGameInstance());
	if (GI == nullptr)
		return false;

	UE_LOG(LogTemp, Warning, TEXT("[Handle_S_USE_SKILL] MyId=%lld FromPlayerId=%lld SkillId=%d ChargeScale=%f"),
		(int64)GI->MyObjectId,
		(int64)pkt.playerid(),
		(int32)pkt.skillid(),
		pkt.chargescale());

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

bool Handle_S_CHANGE_LEVEL(PacketSessionRef& session, Protocol::S_CHANGE_LEVEL& pkt)
{
	FString LevelName = UTF8_TO_TCHAR(pkt.level_name().c_str());

	UE_LOG(LogTemp, Warning, TEXT("[Handle_S_CHANGE_LEVEL] RECEIVED level=%s"), *LevelName);

	AsyncTask(ENamedThreads::GameThread, [LevelName]()
		{
			UE_LOG(LogTemp, Warning, TEXT("[Handle_S_CHANGE_LEVEL] OpenLevel %s"), *LevelName);

			if (UMainGameInstance* GI = GetMainGameInstance())
			{
				GI->MyPlayer = nullptr;
				GI->Players.Empty();

				UE_LOG(LogTemp, Warning, TEXT("[ChangeLevel] Cleared player state before OpenLevel -> %s"), *LevelName);
			}

			if (UWorld* World = GEngine->GetWorldFromContextObject(GetMainGameInstance(), EGetWorldErrorMode::LogAndReturnNull))
			{
				UGameplayStatics::OpenLevel(World, FName(*LevelName));
			}
		});

	return true;
}

bool Handle_S_START_SKILL_CHARGE(PacketSessionRef& session, Protocol::S_START_SKILL_CHARGE& pkt)
{
	if (GWorld == nullptr)
		return false;

	UMainGameInstance* GI = Cast<UMainGameInstance>(GWorld->GetGameInstance());
	if (GI == nullptr)
		return false;

	GI->OnRecvStartSkillCharge(pkt);
	return true;
}