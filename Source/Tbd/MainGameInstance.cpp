// Fill out your copyright notice in the Description page of Project Settings.


#include "MainGameInstance.h"
#include "Sockets.h"
#include "Projectile.h"
#include "MonsterBase.h"
#include "Common/TcpSocketBuilder.h"
#include "Serialization/ArrayWriter.h"
#include "SocketSubsystem.h"
#include "PacketSession.h"
#include "Protocol.pb.h"
#include "ClientPacketHandler.h"
#include "Player/MyPlayerCharacter.h"
#include "Player/UC_NetworkPlayerComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Misc/Paths.h"
#include "Misc/ConfigCacheIni.h"
#include "HAL/PlatformProcess.h"
#include "Misc/CoreDelegates.h"
#include "EngineUtils.h"
#include "Network/NetworkWorker.h"
#include "Kismet/GameplayStatics.h"

void UMainGameInstance::ConnectToGameServer()
{
	if (Socket != nullptr || GameServerSession.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[ConnectToGameServer] Already connected or connecting. Skip. this=%p Socket=%p SessionValid=%d"),
			this,
			Socket,
			GameServerSession.IsValid());
		return;
	}

	FString ConfigIP;
	if (GConfig->GetString(TEXT("Network"), TEXT("ServerIP"), ConfigIP, GGameIni) && !ConfigIP.IsEmpty())
		IpAddress = ConfigIP;

	FString ConfigPortStr;
	if (GConfig->GetString(TEXT("Network"), TEXT("ServerPort"), ConfigPortStr, GGameIni) && ConfigPortStr.IsEmpty() == false)
	{
		const int32 P = FCString::Atoi(*ConfigPortStr);
		if (P > 0 && P < 65536)
			Port = static_cast<int16>(P);
	}

	FString CommandLineIP;
	if (FParse::Value(FCommandLine::Get(), TEXT("TargetIP="), CommandLineIP))
	{
		IpAddress = CommandLineIP;
	}

	UE_LOG(LogTemp, Warning, TEXT("[ConnectToGameServer] Attempting to connect. this=%p IP=%s Port=%d"),
		this,
		*IpAddress,
		Port);

	Players.Empty();
	MyPlayer = nullptr;
	MyObjectId = 0;

	Socket = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(TEXT("Stream"), TEXT("Client Socket"));
	if (Socket == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[ConnectToGameServer] CreateSocket failed"));
		return;
	}

	Socket->SetNonBlocking(true);

	FIPv4Address Ip;
	if (FIPv4Address::Parse(IpAddress, Ip) == false)
	{
		UE_LOG(LogTemp, Error, TEXT("[ConnectToGameServer] Invalid IP: %s"), *IpAddress);

		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Socket);
		Socket = nullptr;
		return;
	}

	TSharedRef<FInternetAddr> InternetAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	InternetAddr->SetIp(Ip.Value);
	InternetAddr->SetPort(Port);

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Connecting To Server..."));

	bool Connected = Socket->Connect(*InternetAddr);

	if (Connected)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Connection Success"));

		GameServerSession = MakeShared<PacketSession>(Socket);
		GameServerSession->SetOwnerGameInstance(this);
		GameServerSession->Run();

		StartRecvPacketsTimer();

		UE_LOG(LogTemp, Warning, TEXT("[ConnectToGameServer] Connected. RecvTimer started. this=%p Socket=%p SessionValid=%d"),
			this,
			Socket,
			GameServerSession.IsValid());

		Protocol::C_LOGIN Pkt;
		SendBufferRef SendBuffer = ClientPacketHandler::MakeSendBuffer(Pkt);
		SendPacket(SendBuffer);
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Connection Failed"));

		UE_LOG(LogTemp, Error, TEXT("[ConnectToGameServer] Connection failed. this=%p"), this);

		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Socket);
		Socket = nullptr;
		GameServerSession.Reset();
	}
}

void UMainGameInstance::DisconnectToGameServer()
{
	StopRecvPacketsTimer();

	if (GameServerSession.IsValid())
	{
		GameServerSession->Disconnect();
		GameServerSession.Reset();
	}

	if (Socket)
	{
		Socket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Socket);
		Socket = nullptr;
	}

	UE_LOG(LogTemp, Warning, TEXT("[DisconnectToGameServer] Done"));
}

void UMainGameInstance::Init()
{
	Super::Init();

	const bool bClientOnly = FParse::Param(FCommandLine::Get(), TEXT("clientonly"));

	if (!bClientOnly)
	{
		FCoreDelegates::OnPreExit.AddUObject(this, &UMainGameInstance::StopServerProcess);
		UE_LOG(LogTemp, Warning, TEXT("[MainGameInstance::Init] Server process started (NOT clientonly)"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[MainGameInstance::Init] clientonly: skip StartServerProcess"));
	}

	ClientPacketHandler::Init();

	UE_LOG(LogTemp, Warning, TEXT("[MainGameInstance::Init] Init complete. RecvPackets timer NOT started here. this=%p"),
		this);
}

void UMainGameInstance::Shutdown()
{
	DisconnectToGameServer();

	StopServerProcess();

	Super::Shutdown();
}

void UMainGameInstance::SendStartSkillCharge(int32 SkillId)
{
	Protocol::C_START_SKILL_CHARGE pkt;
	pkt.set_skillid(SkillId);

	SendBufferRef SendBuffer = ClientPacketHandler::MakeSendBuffer(pkt);
	SendPacket(SendBuffer);
}

void UMainGameInstance::SendMonsterKill()
{
	if (MyObjectId == 0 || Socket == nullptr || !GameServerSession.IsValid())
		return;

	Protocol::C_MONSTER_KILL pkt;
	pkt.set_player_id(MyObjectId);

	SendPacket(ClientPacketHandler::MakeSendBuffer(pkt));

	UE_LOG(LogTemp, Warning, TEXT("[SendMonsterKill] MyObjectId=%llu"), MyObjectId);
}

bool UMainGameInstance::GetPlayerStatsByObjectId(int64 ObjectId, int32& Kill, int32& Death, int32& MonsterKill)
{
	if (FPlayerStatsData* Found = PlayerStatsMap.Find(ObjectId))
	{
		Kill = Found->Kill;
		Death = Found->Death;
		MonsterKill = Found->MonsterKill;

		UE_LOG(LogTemp, Warning, TEXT("[GetPlayerStatsByObjectId FOUND] ObjId=%lld K=%d D=%d M=%d"),
			ObjectId, Kill, Death, MonsterKill);

		return true;
	}

	Kill = 0;
	Death = 0;
	MonsterKill = 0;

	UE_LOG(LogTemp, Warning, TEXT("[GetPlayerStatsByObjectId NOT FOUND] ObjId=%lld"),
		ObjectId);

	return false;
}

void UMainGameInstance::StartServerProcess()
{
	if (bStartedServer && ServerProcHandle.IsValid())
		return;
	const FString ServerExePath =
		FPaths::Combine(FPaths::ProjectContentDir(), TEXT("ExternalServer/GameServer.exe"));

	UE_LOG(LogTemp, Warning, TEXT("ServerExePath: %s"), *ServerExePath);

	if (!FPaths::FileExists(ServerExePath))
	{
		UE_LOG(LogTemp, Error, TEXT("Server exe NOT FOUND"));
		return;
	}

	const FString WorkingDir = FPaths::GetPath(ServerExePath);

	ServerProcHandle = FPlatformProcess::CreateProc(
		*ServerExePath,
		TEXT(""),
		true,
		false,
		false,
		nullptr,
		0,
		*WorkingDir,
		nullptr
	);

	bStartedServer = ServerProcHandle.IsValid();

	UE_LOG(LogTemp, Warning, TEXT("Server started: %s"), bStartedServer ? TEXT("YES") : TEXT("NO"));
}

void UMainGameInstance::StopServerProcess()
{
	if (!bStartedServer)
		return;

	if (ServerProcHandle.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Terminating server process..."));

		FPlatformProcess::TerminateProc(ServerProcHandle, true);
		FPlatformProcess::CloseProc(ServerProcHandle);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ServerProcHandle invalid (nothing to terminate)"));
	}

	bStartedServer = false;
}

void UMainGameInstance::HandleRecvPackets()
{
	if (Socket == nullptr || GameServerSession == nullptr)
	{
		StopRecvPacketsTimer();
		return;
	}

	UE_LOG(LogTemp, VeryVerbose, TEXT("[HandleRecvPackets] this=%p World=%s MyObjectId=%llu"),
		this,
		*GetNameSafe(GetWorld()),
		MyObjectId);

	GameServerSession->HandleRecvPackets();
}

APlayerCharacter* UMainGameInstance::GetPlayerById(uint64 PlayerId)
{
	if (MyPlayer.IsValid() && MyPlayer.Get()->PlayerInfo.object_id() == PlayerId)
	{
		return MyPlayer.Get();
	}

	for (auto& Pair : Players)
	{
		if (Pair.Value.IsValid())
		{
			APlayerCharacter* Player = Pair.Value.Get();
			if (Player && Player->PlayerInfo.object_id() == PlayerId)
			{
				return Player;
			}
		}
	}

	return nullptr;
}

void UMainGameInstance::HandlePlayerStats(const Protocol::S_PLAYER_STATS& pkt)
{
	const int64 ObjectId = static_cast<int64>(pkt.object_id());

	FPlayerStatsData Stats;
	Stats.Kill = pkt.kill_count();
	Stats.Death = pkt.death_count();
	Stats.MonsterKill = pkt.monster_kill_count();

	PlayerStatsMap.Add(ObjectId, Stats);

	if (pkt.object_id() == MyObjectId)
	{
		MyKillCount = pkt.kill_count();
		MyDeathCount = pkt.death_count();
		MyMonsterKillCount = pkt.monster_kill_count();
	}

	UE_LOG(LogTemp, Warning, TEXT("[PlayerStatsMap] ObjId=%lld K=%d D=%d M=%d"),
		ObjectId,
		Stats.Kill,
		Stats.Death,
		Stats.MonsterKill);
}

void UMainGameInstance::HandleGameResult(const Protocol::S_GAME_RESULT& pkt)
{
	bIsWinner = false;
	MyResultScore = 0;
	MyResultKill = 0;
	MyResultDeath = 0;
	MyResultMonsterKill = 0;

	for (const Protocol::GameResultInfo& Result : pkt.results())
	{
		if (Result.object_id() == MyObjectId)
		{
			MyResultKill = Result.kill_count();
			MyResultDeath = Result.death_count();
			MyResultMonsterKill = Result.monster_kill_count();
			MyResultScore = Result.score();
			bIsWinner = Result.is_winner();
			break;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[HandleGameResult] Winner=%d Score=%d K=%d D=%d M=%d"),
		bIsWinner ? 1 : 0,
		MyResultScore,
		MyResultKill,
		MyResultDeath,
		MyResultMonsterKill);

	BP_OnGameResultReceived();
}

void UMainGameInstance::SendPacket(SendBufferRef SendBuffer)
{
	if (Socket == nullptr || GameServerSession == nullptr)
		return;

	GameServerSession->SendPacket(SendBuffer);
}

void UMainGameInstance::SendLevelReady()
{
	Protocol::C_LEVEL_READY pkt;
	auto SendBuffer = ClientPacketHandler::MakeSendBuffer(pkt);
	SendPacket(SendBuffer);

	if (UWorld* World = GetWorld())
	{
		UE_LOG(LogTemp, Warning, TEXT("[SendLevelReady] map=%s MyObjectId=%llu"),
			*World->GetMapName(), MyObjectId);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[SendLevelReady] World nullptr MyObjectId=%llu"), MyObjectId);
	}
}

void UMainGameInstance::DemoChangeLevel(int32 TargetLevel)
{
	Protocol::C_DEMO_NEXT_LEVEL pkt;
	pkt.set_targetlevel(TargetLevel);

	SendBufferRef sendBuffer = ClientPacketHandler::MakeSendBuffer(pkt);
	SendPacket(sendBuffer);
}

void UMainGameInstance::StartRecvPacketsTimer()
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RecvTimer] Start failed. World is nullptr. this=%p"), this);
		return;
	}

	if (World->GetTimerManager().IsTimerActive(RecvPacketsTimerHandle))
	{
		UE_LOG(LogTemp, Warning, TEXT("[RecvTimer] Already active. Skip. this=%p World=%s"),
			this,
			*GetNameSafe(World));
		return;
	}

	World->GetTimerManager().SetTimer(
		RecvPacketsTimerHandle,
		this,
		&UMainGameInstance::HandleRecvPackets,
		0.01f,
		true
	);

	UE_LOG(LogTemp, Warning, TEXT("[RecvTimer] Started. this=%p World=%s"),
		this,
		*GetNameSafe(World));
}

void UMainGameInstance::StopRecvPacketsTimer()
{
	UWorld* World = GetWorld();
	if (World == nullptr)
		return;

	World->GetTimerManager().ClearTimer(RecvPacketsTimerHandle);

	UE_LOG(LogTemp, Warning, TEXT("[RecvTimer] Stopped. this=%p World=%s"),
		this,
		*GetNameSafe(World));
}

void UMainGameInstance::HandleSpawn(const Protocol::PlayerInfo& PlayerInfo, bool IsMine)
{
	if (bChangingLevel)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HandleSpawn] Deferred: changing level objId=%llu"), PlayerInfo.object_id());
		PendingSpawns.Add(PlayerInfo);
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
		return;

	const uint64 ObjectId = PlayerInfo.object_id();
	FVector SpawnLocation(PlayerInfo.x(), PlayerInfo.y(), PlayerInfo.z());
	FRotator SpawnRotation(0.f, PlayerInfo.yaw(), 0.f);

	const bool bZeroSpawn =
		FMath::IsNearlyZero(SpawnLocation.X) &&
		FMath::IsNearlyZero(SpawnLocation.Y) &&
		FMath::IsNearlyZero(SpawnLocation.Z);

	UE_LOG(LogTemp, Warning, TEXT("[HandleSpawn] objId=%llu isMine=%d loc=(%.1f, %.1f, %.1f)"),
		ObjectId, IsMine ? 1 : 0, SpawnLocation.X, SpawnLocation.Y, SpawnLocation.Z);

	if (IsMine)
	{
		APlayerCharacter* LocalPlayer = nullptr;

		if (MyPlayer.IsValid())
		{
			LocalPlayer = MyPlayer.Get();
		}
		else
		{
			APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
			if (!PC)
			{
				UE_LOG(LogTemp, Warning, TEXT("[HandleSpawn] PC nullptr"));
				return;
			}

			APawn* Pawn = PC->GetPawn();
			if (!Pawn)
			{
				UE_LOG(LogTemp, Warning, TEXT("[HandleSpawn] Pawn nullptr"));
				return;
			}

			LocalPlayer = Cast<APlayerCharacter>(Pawn);
			if (!LocalPlayer)
			{
				UE_LOG(LogTemp, Warning, TEXT("[HandleSpawn] LocalPlayer cast failed"));
				return;
			}

			MyPlayer = LocalPlayer;
		}

		if (TWeakObjectPtr<APlayerCharacter>* FoundPtr = Players.Find(ObjectId))
		{
			if (FoundPtr->IsValid() && FoundPtr->Get() != LocalPlayer)
			{
				UE_LOG(LogTemp, Warning, TEXT("[HandleSpawn] Destroy duplicate actor for my objId=%llu actor=%s"),
					ObjectId, *GetNameSafe(FoundPtr->Get()));

				FoundPtr->Get()->Destroy();
			}
		}

		LocalPlayer->bIsMine = true;
		LocalPlayer->SetPlayerInfo(PlayerInfo);

		LocalPlayer->SetActorLocation(SpawnLocation);
		LocalPlayer->SetActorRotation(SpawnRotation);

		UUpgradeComponent* UpgradeComp = LocalPlayer->FindComponentByClass<UUpgradeComponent>();
		if (UpgradeComp)
		{
			UpgradeComp->SetUpgrades(LocalPlayerUpgradeMap);
			UE_LOG(LogTemp, Warning, TEXT("[HandleSpawn] Applied upgrades for my objId=%llu"), ObjectId);
		}

		/*if (UCharacterMovementComponent* MoveComp = LocalPlayer->GetCharacterMovement())
		{
			MoveComp->StopMovementImmediately();
		}*/

		MyObjectId = ObjectId;
		Players.FindOrAdd(ObjectId) = LocalPlayer;

		UE_LOG(LogTemp, Warning, TEXT("[HandleSpawn] MyPlayer set/update objId=%llu loc=(%.1f, %.1f, %.1f)"),
			ObjectId, SpawnLocation.X, SpawnLocation.Y, SpawnLocation.Z);
		return;
	}

	if (ObjectId == MyObjectId && MyObjectId != 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HandleSpawn] Skip other spawn for my objId=%llu"), ObjectId);
		return;
	}

	APlayerCharacter* TargetActor = nullptr;

	if (TWeakObjectPtr<APlayerCharacter>* FoundPtr = Players.Find(ObjectId))
	{
		if (FoundPtr->IsValid())
		{
			TargetActor = FoundPtr->Get();
		}
		else
		{
			Players.Remove(ObjectId);
		}
	}

	if (TargetActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HandleSpawn] Update existing other objId=%llu"), ObjectId);

		TargetActor->bIsMine = false;
		TargetActor->SetPlayerInfo(PlayerInfo);

		if (!bZeroSpawn)
		{
			TargetActor->SetActorLocation(SpawnLocation);
			TargetActor->SetActorRotation(SpawnRotation);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[HandleSpawn] Ignore zero update objId=%llu"), ObjectId);
		}
	}
	else if (OtherPlayerClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HandleSpawn] Spawn new other objId=%llu"), ObjectId);

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		APlayerCharacter* NewOther = World->SpawnActor<APlayerCharacter>(OtherPlayerClass, SpawnLocation, SpawnRotation, SpawnParams);
		if (NewOther)
		{
			NewOther->bIsMine = false;
			NewOther->SetPlayerInfo(PlayerInfo);
			Players.Add(ObjectId, NewOther);
		}
	}
}

void UMainGameInstance::HandleSpawn(const Protocol::S_ENTER_GAME& EnterGamePkt)
{
	if (MyObjectId != 0)
		return;

	MyObjectId = EnterGamePkt.player().object_id();

	HandleSpawn(EnterGamePkt.player(), true);
}

void UMainGameInstance::HandleSpawn(const Protocol::S_SPAWN& SpawnPkt)
{
	for (const auto& PlayerInfo : SpawnPkt.players())
	{
		HandleSpawn(PlayerInfo, false);
	}
}

void UMainGameInstance::HandleDespawn(uint64 ObjectId)
{
	if (TWeakObjectPtr<APlayerCharacter>* Found = Players.Find(ObjectId))
	{
		if (Found->IsValid())
		{
			Found->Get()->Destroy();
		}
	}

	Players.Remove(ObjectId);

	if (MyObjectId == ObjectId)
	{
		MyPlayer.Reset();
		MyObjectId = 0;
	}
}

void UMainGameInstance::HandleDespawn(const Protocol::S_DESPAWN& DespawnPkt)
{
	for (auto& ObjectId : DespawnPkt.object_ids())
	{
		HandleDespawn(ObjectId);
	}
}

void UMainGameInstance::HandleMove(const Protocol::S_MOVE& MovePkt)
{
	if (Socket == nullptr || GameServerSession == nullptr)
		return;

	auto* World = GetWorld();
	if (World == nullptr)
		return;

	const uint64 ObjectId = MovePkt.info().object_id();

	TWeakObjectPtr<APlayerCharacter>* FindActor = Players.Find(ObjectId);
	if (FindActor == nullptr)
		return;

	if (!FindActor->IsValid())
	{
		Players.Remove(ObjectId);
		return;
	}

	APlayerCharacter* Player = FindActor->Get();
	if (Player == nullptr)
		return;

	Player->SetDestInfo(MovePkt.info());
}

void UMainGameInstance::HandleDespawnMob(uint64 MobId)
{
	auto* World = GetWorld();
	if (World == nullptr)
		return;

	AActor** Found = Monsters.Find(MobId);
	if (Found == nullptr)
		return;

	AActor* MobActor = *Found;
	Monsters.Remove(MobId);

	if (MobActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("Mob Despawned: ID=%llu"), MobId);
		World->DestroyActor(MobActor);
	}
}

void UMainGameInstance::HandleDespawnMob(const Protocol::S_DESPAWN_MOB& Pkt)
{
	for (uint64 MobId : Pkt.mobids())
	{
		HandleDespawnMob(MobId);
	}
}

void UMainGameInstance::HandleMoveMob(const Protocol::S_MOVE_MOB& Pkt)
{
	const Protocol::MobInfo& MobInfo = Pkt.mob();
	const uint64 MobId = MobInfo.mobid();
	AActor** Found = Monsters.Find(MobId);
	if (Found == nullptr)
		return;

	AActor* MobActor = *Found;
	if (MobActor == nullptr)
		return;

	// Character인 경우 XY만 업데이트하고 Z는 중력에 맡김
	if (ACharacter* Character = Cast<ACharacter>(MobActor))
	{
		FVector CurrentPos = Character->GetActorLocation();
		FVector NewPos(MobInfo.pos().x(), MobInfo.pos().y(), CurrentPos.Z); // Z는 현재 위치 유지
		
		// XY만 업데이트 (Z는 중력이 처리)
		Character->SetActorLocation(NewPos, false, nullptr, ETeleportType::None);
	}
	else
	{
		// Character가 아닌 경우 전체 위치 업데이트
		FVector NewPos(MobInfo.pos().x(), MobInfo.pos().y(), MobInfo.pos().z());
		MobActor->SetActorLocation(NewPos);
	}
}

void UMainGameInstance::SendAttackMob(int64 MobId)
{
	if (Socket == nullptr || GameServerSession == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SendAttackMob] Socket or Session is nullptr"));
		return;
	}

	uint64 mobIdUint64 = static_cast<uint64>(MobId);

	Protocol::C_ATTACK_MOB attackPkt;
	attackPkt.set_mobid(mobIdUint64);

	SendBufferRef sendBuffer = ClientPacketHandler::MakeSendBuffer(attackPkt);
	SendPacket(sendBuffer);

	UE_LOG(LogTemp, Warning, TEXT("[SendAttackMob] Sent attack packet for Mob ID=%llu"), mobIdUint64);
}

void UMainGameInstance::HandleDamageMob(const Protocol::S_DAMAGE_MOB& Pkt)
{
	const uint64 MobId = Pkt.mobid();
	const int32 Damage = Pkt.damage();
	const int32 Hp = Pkt.hp();

	if (Monsters.Contains(MobId))
	{
		UE_LOG(LogTemp, Warning, TEXT("Mob Damaged: ID=%llu Damage=%d HP=%d"), MobId, Damage, Hp);
	}
}

void UMainGameInstance::OnRecvProjectileHit(const Protocol::S_PROJECTILE_HIT& pkt)
{
}

void UMainGameInstance::OnRecvProjectileDestroy(const Protocol::S_PROJECTILE_DESTROY& pkt)
{
	const int32 ProjectileId = (int32)pkt.projectileid();

	if (AActor** Found = ByProjectileId.Find(ProjectileId))
	{
		if (AActor* Proj = *Found)
			Proj->Destroy();

		ByProjectileId.Remove(ProjectileId);
	}
}

void UMainGameInstance::SendUseSkill(int32 SkillId, float ChargeScale)
{
	if (MyObjectId == 0 || !MyPlayer.IsValid())
		return;

	Protocol::C_USE_SKILL pkt;
	pkt.set_playerid(MyObjectId);
	pkt.set_skillid(SkillId);

	pkt.set_chargescale(ChargeScale);

	pkt.set_clientshotid(0);

	Protocol::Vector3* Dir = pkt.mutable_dir();
	Dir->set_x(0.f);
	Dir->set_y(0.f);
	Dir->set_z(0.f);

	uint64 TargetId = 0;
	AActor* SkillTarget = MyPlayer->LockedTargetActor;

	if (SkillId == 0)
	{
		LastLocalIceTarget = SkillTarget;

		UE_LOG(LogTemp, Warning, TEXT("[SendUseSkill] Save LastLocalIceTarget=%s"),
			*GetNameSafe(LastLocalIceTarget));
	}

	if (SkillTarget)
	{
		if (APlayerCharacter* TargetPlayer = Cast<APlayerCharacter>(SkillTarget))
		{
			TargetId = TargetPlayer->PlayerInfo.object_id();
		}
		else if (AMonsterBase* TargetMonster = Cast<AMonsterBase>(SkillTarget))
		{
			for (const auto& Pair : Monsters)
			{
				if (Pair.Value == TargetMonster)
				{
					TargetId = Pair.Key;
					break;
				}
			}
		}
	}

	pkt.set_targetid(TargetId);

	UE_LOG(LogTemp, Warning, TEXT("[SendUseSkill] SkillId=%d ChargeScale=%f LockedTarget=%s TargetId=%llu"),
		SkillId,
		ChargeScale,
		*GetNameSafe(SkillTarget),
		TargetId);

	SendPacket(ClientPacketHandler::MakeSendBuffer(pkt));
}

void UMainGameInstance::SendEnterGamePacket()
{
	if (!GameServerSession.IsValid())
		return;

	Protocol::C_ENTER_GAME EnterGamePkt;
	EnterGamePkt.set_playerindex(0);

	auto SendBuffer = ClientPacketHandler::MakeSendBuffer(EnterGamePkt);
	GameServerSession->SendPacket(SendBuffer);

	UE_LOG(LogTemp, Warning, TEXT("[SendEnterGamePacket] Sent C_ENTER_GAME"));
}

void UMainGameInstance::ClearPlayerStateForLevelChange()
{
	bChangingLevel = true;

	MyPlayer.Reset();
	Players.Empty();
	PendingSpawns.Empty();

	UE_LOG(LogTemp, Warning, TEXT("[ClearPlayerStateForLevelChange] Cleared player refs (MyObjectId=%llu kept)"), MyObjectId);
}

void UMainGameInstance::NotifyLevelLoadFinished()
{
	bChangingLevel = false;
	UE_LOG(LogTemp, Warning, TEXT("[NotifyLevelLoadFinished] Level load finished. PendingSpawns=%d"), PendingSpawns.Num());

	TArray<Protocol::PlayerInfo> SavedSpawns = PendingSpawns;
	PendingSpawns.Empty();

	for (const auto& Info : SavedSpawns)
	{
		HandleSpawn(Info, false);
	}
}

void UMainGameInstance::StartGameConnection()
{
	if (GameServerSession.IsValid() || Socket != nullptr)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[StartGameConnection] Begin connect"));
	ConnectToGameServer();
}

void UMainGameInstance::HandleDamage(const Protocol::S_DAMAGE_PLAYER& pkt)
{
	uint64 ObjectId = pkt.object_id();
	float Damage = pkt.damage();

	APlayerCharacter* Actor = nullptr;


	UE_LOG(LogTemp, Warning, TEXT("[HandleDamage APPLY] this=%p World=%s Actor=%s ObjId=%llu Damage=%f"),
		this,
		*GetNameSafe(GetWorld()),
		*GetNameSafe(Actor),
		ObjectId,
		Damage);

	// 내가 맞은 경우
	if (ObjectId == MyObjectId)
	{
		Actor = MyPlayer.Get();

		if (Actor == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("[HandleDamage] MyPlayer is null ObjId=%llu"), ObjectId);
			return;
		}
	}
	// 다른 플레이어가 맞은 경우
	else
	{
		TWeakObjectPtr<APlayerCharacter>* FoundActor = Players.Find(ObjectId);
		if (FoundActor == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("[HandleDamage] Other player not found in map ObjId=%llu"), ObjectId);
			return;
		}

		Actor = FoundActor->Get();
		if (Actor == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("[HandleDamage] Invalid other actor, remove stale entry ObjId=%llu"), ObjectId);
			Players.Remove(ObjectId);
			return;
		}
	}

	if (Actor->AttributeComponent == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HandleDamage] AttributeComponent is null Actor=%s"),
			*GetNameSafe(Actor));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[HandleDamage] ApplyDamage Actor=%s ObjId=%llu Damage=%f"),
		*GetNameSafe(Actor), ObjectId, Damage);

	Actor->AttributeComponent->SubtractHealth(Damage);
	Actor->PlayHitReaction();
}

void UMainGameInstance::HandleDie(const Protocol::S_PLAYER_DEAD& Pkt)
{
	uint64 ObjectId = Pkt.object_id();

	UE_LOG(LogTemp, Warning, TEXT("[HandleDie] this=%p ObjId=%llu Players.Num=%d"),
		this, ObjectId, Players.Num());

	TWeakObjectPtr<APlayerCharacter>* FoundActor = Players.Find(ObjectId);
	if (FoundActor == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HandleDie] Player not found. ObjId=%llu"), ObjectId);
		return;
	}

	APlayerCharacter* TargetCharacter = FoundActor->Get();
	if (TargetCharacter == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HandleDie] TargetCharacter nullptr. ObjId=%llu"), ObjectId);
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[HandleDie] Actor=%s IsMine=%d"),
		*GetNameSafe(TargetCharacter), TargetCharacter->bIsMine ? 1 : 0);

	TargetCharacter->SetDead(true);
}

void UMainGameInstance::SendAttackPlayer(uint64 TargetId, uint32 SkillId)
{
	UE_LOG(LogTemp, Warning, TEXT("[SendAttackPlayer] target=%llu skill=%d Socket=%d Session=%d"),
		TargetId, SkillId,
		Socket != nullptr ? 1 : 0,
		GameServerSession != nullptr ? 1 : 0);

	Protocol::C_ATTACK_PLAYER pkt;
	pkt.set_targetplayerid(TargetId);
	pkt.set_skillid(SkillId);

	SendPacket(ClientPacketHandler::MakeSendBuffer(pkt));

	UE_LOG(LogTemp, Warning, TEXT("[Client] SendAttackPlayer target=%llu skill=%d"),
		TargetId, SkillId);
}

void UMainGameInstance::OnRecvUseSkill(const Protocol::S_USE_SKILL& pkt)
{
	const uint64 PlayerId = pkt.playerid();
	const int32 SkillId = pkt.skillid();
	const uint64 TargetId = pkt.targetid();
	const float ChargeScale = pkt.chargescale();

	UE_LOG(LogTemp, Warning, TEXT("[OnRecvUseSkill ENTER] PlayerId=%llu MyObjectId=%llu SkillId=%d TargetId=%llu ChargeScale=%f"),
		(unsigned long long)PlayerId,
		(unsigned long long)MyObjectId,
		SkillId,
		(unsigned long long)TargetId,
		ChargeScale);

	APlayerCharacter* Player = GetPlayerById(PlayerId);
	if (Player == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[OnRecvUseSkill RETURN] Player not found PlayerId=%llu"),
			(unsigned long long)PlayerId);
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[OnRecvUseSkill PLAYER FOUND] Player=%s IsMine=%d"),
		*GetNameSafe(Player),
		Player->IsMyPlayer() ? 1 : 0);

	if (!Player->IsMyPlayer())
	{
		UE_LOG(LogTemp, Warning, TEXT("[OnRecvUseSkill] PlayOtherPlayerSkill SkillId=%d"), SkillId);
		Player->PlayOtherPlayerSkill(SkillId);
	}

	switch (SkillId)
	{
	case 0:
		UE_LOG(LogTemp, Warning, TEXT("[OnRecvUseSkill BRANCH] Ice"));
		HandleIceSkillPacket(PlayerId, TargetId);
		break;

	case 1:
		UE_LOG(LogTemp, Warning, TEXT("[OnRecvUseSkill BRANCH] Fireball"));
		HandleFireballSkillPacket(PlayerId, ChargeScale);
		break;

	default:
		UE_LOG(LogTemp, Warning, TEXT("[OnRecvUseSkill BRANCH] Unknown SkillId=%d"), SkillId);
		break;
	}

	UE_LOG(LogTemp, Warning, TEXT("[OnRecvUseSkill END] PlayerId=%llu SkillId=%d"),
		(unsigned long long)PlayerId,
		SkillId);
}

void UMainGameInstance::OnRecvStartSkillCharge(const Protocol::S_START_SKILL_CHARGE& pkt)
{
	const uint64 PlayerId = pkt.playerid();
	const int32 SkillId = pkt.skillid();

	APlayerCharacter* Player = GetPlayerById(PlayerId);
	if (Player == nullptr)
		return;

	if (Player->IsMyPlayer())
		return;

	Player->PlayOtherPlayerHoldSkill(SkillId);
}

void UMainGameInstance::HandleIceSkillPacket(uint64 CasterID, uint64 TargetID)
{
	APlayerCharacter* Caster = GetPlayerById(CasterID);
	if (Caster == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("HandleIceSkillPacket: Caster not found (ID: %lld)"), (int64)CasterID);
		return;
	}

	AActor* Target = GetPlayerById(TargetID);

	if (Target == nullptr && Monsters.Contains(TargetID))
	{
		Target = Monsters[TargetID];
	}

	if (Target == nullptr && CasterID == MyObjectId)
	{
		Target = LastLocalIceTarget;

		UE_LOG(LogTemp, Warning, TEXT("[HandleIceSkillPacket] Use LastLocalIceTarget=%s"),
			*GetNameSafe(Target));
	}

	if (Target == nullptr && TargetID != 0)
	{
		for (TActorIterator<AMonsterBase> It(GetWorld()); It; ++It)
		{
			AMonsterBase* Monster = *It;
			if (Monster && static_cast<uint64>(Monster->MonsterId) == TargetID)
			{
				Target = Monster;
				break;
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[HandleIceSkillPacket] CasterID=%llu TargetID=%llu Target=%s"),
		(unsigned long long)CasterID,
		(unsigned long long)TargetID,
		*GetNameSafe(Target));

	if (IceProjectileBPClass == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[HandleIceSkillPacket] IceProjectileBPClass is nullptr"));
		return;
	}

	FVector Forward = Caster->GetActorForwardVector();
	FVector Right = Caster->GetActorRightVector();

	FVector BaseLocation = Caster->GetActorLocation() + Forward * 100.0f;

	float CurrentTime = GetWorld()->GetTimeSeconds();

	bool bUseRightHand = true;

	bool* SavedNextHand = NextIceRightHandMap.Find(CasterID);
	float* SavedLastTime = LastIceFireTimeMap.Find(CasterID);

	if (SavedNextHand != nullptr && SavedLastTime != nullptr)
	{
		float DeltaTime = CurrentTime - *SavedLastTime;

		if (DeltaTime <= IceComboResetTime)
		{
			bUseRightHand = *SavedNextHand;
		}
		else
		{
			bUseRightHand = true;
		}

		UE_LOG(LogTemp, Warning, TEXT("[IceCombo] CasterID=%llu DeltaTime=%f ResetTime=%f"),
			(unsigned long long)CasterID,
			DeltaTime,
			IceComboResetTime);
	}
	else
	{
		bUseRightHand = true;

		UE_LOG(LogTemp, Warning, TEXT("[IceCombo] CasterID=%llu First Ice"),
			(unsigned long long)CasterID);
	}

	FVector SpawnLocation;
	const TCHAR* HandName = bUseRightHand ? TEXT("Right") : TEXT("Left");

	if (bUseRightHand)
	{
		SpawnLocation = BaseLocation + Right * 35.0f;
	}
	else
	{
		SpawnLocation = BaseLocation - Right * 35.0f;
	}

	LastIceFireTimeMap.Add(CasterID, CurrentTime);
	NextIceRightHandMap.Add(CasterID, !bUseRightHand);

	FRotator SpawnRotation = Caster->GetActorRotation();
	FTransform SpawnTransform(SpawnRotation, SpawnLocation);

	AActor* SpawnedActor = GetWorld()->SpawnActorDeferred<AActor>(IceProjectileBPClass, SpawnTransform);
	if (SpawnedActor == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[HandleIceSkillPacket] SpawnActorDeferred failed"));
		return;
	}

	if (FObjectProperty* Prop = FindFProperty<FObjectProperty>(SpawnedActor->GetClass(), TEXT("TargetActor")))
	{
		Prop->SetPropertyValue_InContainer(SpawnedActor, Target);
	}

	if (FBoolProperty* bMineProp = FindFProperty<FBoolProperty>(SpawnedActor->GetClass(), TEXT("bIsHomingSkillMine")))
	{
		bMineProp->SetPropertyValue_InContainer(SpawnedActor, Caster->IsMyPlayer());
	}

	AProjectile* Projectile = Cast<AProjectile>(SpawnedActor);
	if (Projectile)
	{
		Projectile->SetProjectileInfo(0, Caster);
		Projectile->SetHomingTarget(Target);

		UE_LOG(LogTemp, Warning, TEXT("[HandleIceSkillPacket] %s SetProjectileInfo Owner=%s SkillId=%d Target=%s"),
			HandName,
			*GetNameSafe(Caster),
			0,
			*GetNameSafe(Target));
	}

	SpawnedActor->FinishSpawning(SpawnTransform);

	if (Projectile)
	{
		Projectile->ActivateProjectileCollision();
		Projectile->LaunchProjectile(Forward);

		UE_LOG(LogTemp, Warning, TEXT("[HandleIceSkillPacket] %s Ice Launch Forward=%s"),
			HandName,
			*Forward.ToString());
	}
}

void UMainGameInstance::HandleFireballSkillPacket(uint64 CasterID, float ChargeScale)
{
	APlayerCharacter* Caster = GetPlayerById(CasterID);
	if (Caster == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HandleFireballSkillPacket] Caster is null CasterID=%llu"),
			(unsigned long long)CasterID);
		return;
	}

	if (FireballProjectileBPClass == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HandleFireballSkillPacket] FireballProjectileBPClass is null"));
		return;
	}

	ChargeScale = FMath::Clamp(ChargeScale, 1.0f, 3.0f);

	FVector Forward = Caster->GetActorForwardVector().GetSafeNormal();
	if (Forward.IsNearlyZero())
	{
		Forward = Caster->GetActorRotation().Vector().GetSafeNormal();
	}

	FVector SpawnLocation =
		Caster->GetMesh()->GetSocketLocation(TEXT("RightHandSpellSocket"))
		+ Forward * 80.f;

	FRotator SpawnRotation = Forward.Rotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Caster;
	SpawnParams.Instigator = Caster;

	AProjectile* Projectile = GetWorld()->SpawnActor<AProjectile>(
		FireballProjectileBPClass,
		SpawnLocation,
		SpawnRotation,
		SpawnParams
	);

	if (Projectile == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HandleFireballSkillPacket] Projectile spawn failed"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[HandleFireballSkillPacket] Spawned Projectile=%s ChargeScale=%f Location=%s Forward=%s"),
		*GetNameSafe(Projectile),
		ChargeScale,
		*SpawnLocation.ToString(),
		*Forward.ToString());

	Projectile->SetProjectileInfo(1, Caster);
	Projectile->SetChargeScale(ChargeScale);

	Projectile->LaunchProjectile(Forward);
}
