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

	UE_LOG(LogTemp, Warning, TEXT("[Handle_S_ENTER_GAME] Received ObjId=%llu Loc=(%.1f, %.1f, %.1f)"),
		(unsigned long long)PlayerCopy.object_id(),
		PlayerCopy.x(),
		PlayerCopy.y(),
		PlayerCopy.z());

	if (UMainGameInstance* GI = session->GetOwnerGameInstance())
	{
		AsyncTask(ENamedThreads::GameThread, [GI, PlayerCopy]()
			{
				if (GI == nullptr)
					return;

				UE_LOG(LogTemp, Warning, TEXT("[Handle_S_ENTER_GAME GameThread] bChangingLevel=%d ObjId=%llu Loc=(%.1f, %.1f, %.1f)"),
					GI->bChangingLevel ? 1 : 0,
					(unsigned long long)PlayerCopy.object_id(),
					PlayerCopy.x(),
					PlayerCopy.y(),
					PlayerCopy.z());

				GI->MyObjectId = PlayerCopy.object_id();

				UE_LOG(LogTemp, Error, TEXT("[Handle_S_ENTER_GAME] SET MyObjectId=%llu Loc=(%.1f, %.1f, %.1f)"),
					(unsigned long long)GI->MyObjectId,
					PlayerCopy.x(),
					PlayerCopy.y(),
					PlayerCopy.z());

				if (GI->bChangingLevel)
				{
					UE_LOG(LogTemp, Warning, TEXT("[Handle_S_ENTER_GAME] Finish level transition before HandleSpawn"));
					GI->NotifyLevelLoadFinished();
				}

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
	Protocol::S_SPAWN SpawnCopy = pkt;

	for (const auto& Info : SpawnCopy.players())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Handle_S_SPAWN] Received ObjId=%llu Loc=(%.1f, %.1f, %.1f)"),
			(unsigned long long)Info.object_id(),
			Info.x(),
			Info.y(),
			Info.z());
	}

	if (UMainGameInstance* GI = session->GetOwnerGameInstance())
	{
		AsyncTask(ENamedThreads::GameThread, [GI, SpawnCopy]()
			{
				if (GI == nullptr)
					return;

				UE_LOG(LogTemp, Warning, TEXT("[Handle_S_SPAWN GameThread] bChangingLevel=%d MyObjectId=%llu players=%d"),
					GI->bChangingLevel ? 1 : 0,
					(unsigned long long)GI->MyObjectId,
					SpawnCopy.players_size());

				GI->HandleSpawn(SpawnCopy);
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
	const uint64 ObjId = pkt.info().object_id();

	UE_LOG(LogTemp, Warning, TEXT("[Handle_S_MOVE] packet received objId=%llu"),
		static_cast<unsigned long long>(ObjId));

	if (auto* GameInstance = GetMainGameInstance())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Handle_S_MOVE] GameInstance valid MyObjectId=%llu"),
			static_cast<unsigned long long>(GameInstance->MyObjectId));

		if (ObjId == GameInstance->MyObjectId)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Handle_S_MOVE] Ignore my own move ObjId=%llu"),
				static_cast<unsigned long long>(ObjId));
			return true;
		}

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

	Protocol::S_GAME_RESULT ResultCopy = pkt;

	AsyncTask(ENamedThreads::GameThread, [GameInstance, ResultCopy]()
		{
			if (GameInstance == nullptr)
				return;

			UE_LOG(LogTemp, Warning, TEXT("[Handle_S_GAME_RESULT GameThread] Call HandleGameResult MyObjectId=%llu ResultCount=%d"),
				(unsigned long long)GameInstance->MyObjectId,
				ResultCopy.results_size());

			GameInstance->HandleGameResult(ResultCopy);
		});

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
	if (session == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[Handle_S_USE_SKILL] session nullptr"));
		return false;
	}

	UMainGameInstance* GI = session->GetOwnerGameInstance();
	if (GI == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[Handle_S_USE_SKILL] OwnerGameInstance nullptr"));
		return false;
	}

	Protocol::S_USE_SKILL PktCopy = pkt;

	AsyncTask(ENamedThreads::GameThread, [GI, PktCopy]()
		{
			if (GI == nullptr)
				return;

			UE_LOG(LogTemp, Warning, TEXT("[Handle_S_USE_SKILL GameThread] MyId=%llu FromPlayerId=%llu SkillId=%d TargetId=%llu ChargeScale=%f"),
				(unsigned long long)GI->MyObjectId,
				(unsigned long long)PktCopy.playerid(),
				(int32)PktCopy.skillid(),
				(unsigned long long)PktCopy.targetid(),
				PktCopy.chargescale());

			GI->OnRecvUseSkill(PktCopy);
		});

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

	UE_LOG(LogTemp, Warning, TEXT("[Handle_S_CHANGE_LEVEL] RECEIVED level=%s session=%p"),
		*LevelName,
		session.Get());

	if (session == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[Handle_S_CHANGE_LEVEL] session nullptr"));
		return true;
	}

	UMainGameInstance* GI = session->GetOwnerGameInstance();
	if (GI == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[Handle_S_CHANGE_LEVEL] OwnerGameInstance nullptr"));
		return true;
	}

	AsyncTask(ENamedThreads::GameThread, [GI, LevelName]()
		{
			if (GI == nullptr)
				return;

			UE_LOG(LogTemp, Warning, TEXT("[Handle_S_CHANGE_LEVEL GameThread] GI=%p MyObjectId=%llu OpenLevel=%s"),
				GI,
				(unsigned long long)GI->MyObjectId,
				*LevelName);

			GI->ClearPlayerStateForLevelChange();

			GI->ResetLevelTransitionState();

			GI->BeginLevelTransition();

			UWorld* World = GI->GetWorld();
			if (World == nullptr)
			{
				UE_LOG(LogTemp, Error, TEXT("[Handle_S_CHANGE_LEVEL] GI->GetWorld nullptr GI=%p"), GI);
				GI->AbortLevelTransition();
				return;
			}

			UGameplayStatics::OpenLevel(World, FName(*LevelName));
		});

	return true;
}

bool Handle_S_START_SKILL_CHARGE(PacketSessionRef& session, Protocol::S_START_SKILL_CHARGE& pkt)
{
	if (session == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[Handle_S_START_SKILL_CHARGE] session nullptr"));
		return false;
	}

	UMainGameInstance* GI = session->GetOwnerGameInstance();
	if (GI == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[Handle_S_START_SKILL_CHARGE] OwnerGameInstance nullptr"));
		return false;
	}

	Protocol::S_START_SKILL_CHARGE PktCopy = pkt;

	AsyncTask(ENamedThreads::GameThread, [GI, PktCopy]()
		{
			if (GI == nullptr)
				return;

			GI->OnRecvStartSkillCharge(PktCopy);
		});

	return true;
}