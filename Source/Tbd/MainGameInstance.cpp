// Fill out your copyright notice in the Description page of Project Settings.


#include "MainGameInstance.h"
#include "Sockets.h"
#include "Projectile.h"
#include "Common/TcpSocketBuilder.h"
#include "Serialization/ArrayWriter.h"
#include "SocketSubsystem.h"
#include "PacketSession.h"
#include "Protocol.pb.h"
#include "ClientPacketHandler.h"
#include "Player/MyPlayerCharacter.h"
#include "Player/UC_NetworkPlayerComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Misc/Paths.h"
#include "Misc/ConfigCacheIni.h"
#include "HAL/PlatformProcess.h"
#include "Misc/CoreDelegates.h"
#include "EngineUtils.h"

void UMainGameInstance::ConnectToGameServer()
{
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

	UE_LOG(LogTemp, Warning, TEXT("Attempting to connect to: %s"), *IpAddress);

	Players.Empty();
	MyPlayer = nullptr;
	MyObjectId = 0;

	Socket = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(TEXT("Stream"), TEXT("Client Socket"));
	Socket->SetNonBlocking(true);

	FIPv4Address Ip;
	FIPv4Address::Parse(IpAddress, Ip);

	TSharedRef<FInternetAddr> InternetAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	InternetAddr->SetIp(Ip.Value);
	InternetAddr->SetPort(Port);

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Connecting To Server...")));

	bool Connected = Socket->Connect(*InternetAddr);

	if (Connected)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Connection Success")));

		// Session
		GameServerSession = MakeShared<PacketSession>(Socket);
		GameServerSession->Run();

		{
			Protocol::C_LOGIN Pkt;
			SendBufferRef SendBuffer = ClientPacketHandler::MakeSendBuffer(Pkt);
			SendPacket(SendBuffer);
		}
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Connection Failed")));
	}
}

void UMainGameInstance::DisconnectToGameServer()
{

	if ( Socket == nullptr || GameServerSession == nullptr )
		return;

	Protocol::C_LEAVE_GAME LeavePkt;
	SEND_PACKET(LeavePkt);
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

	//ConnectToGameServer();

	if (UWorld* World = GetWorld()) 
	{
		World->GetTimerManager().SetTimer(
			RecvPacketsTimerHandle,
			this,
			&UMainGameInstance::HandleRecvPackets,
			0.01f,
			true
		);
	}

	UE_LOG(LogTemp, Warning, TEXT("[MainGameInstance::Init] RecvPackets timer started"));
}

void UMainGameInstance::Shutdown()
{
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
	if ( Socket == nullptr || GameServerSession == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HandleRecvPackets] Socket or Session is nullptr"));
		return;
	}
	
	UE_LOG(LogTemp, VeryVerbose, TEXT("[HandleRecvPackets] Calling HandleRecvPackets..."));
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
			//UpgradeComp->SetUpgrades(LocalPlayerUpgradeMap);
		}

		if (UCharacterMovementComponent* MoveComp = LocalPlayer->GetCharacterMovement())
		{
			MoveComp->StopMovementImmediately();
		}

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

void UMainGameInstance::SendUseSkill(int32 SkillId)
{
	if (MyObjectId == 0 || !MyPlayer.IsValid())
		return;

	Protocol::C_USE_SKILL pkt;
	pkt.set_playerid(MyObjectId);
	pkt.set_skillid(SkillId);

	pkt.set_chargescale(CurrentFireballChargeScale);

	pkt.set_clientshotid(0);

	Protocol::Vector3* Dir = pkt.mutable_dir();
	Dir->set_x(0.f);
	Dir->set_y(0.f);
	Dir->set_z(0.f);

	if (MyPlayer->LockedTargetActor)
	{
		APlayerCharacter* TargetChar = Cast<APlayerCharacter>(MyPlayer->LockedTargetActor);
		if (TargetChar)
		{
			pkt.set_targetid(TargetChar->PlayerInfo.object_id());
		}
		else
		{
			pkt.set_targetid(0);
		}
	}
	else
	{
		pkt.set_targetid(0);
	}

	SendPacket(ClientPacketHandler::MakeSendBuffer(pkt));

	UE_LOG(LogTemp, Warning, TEXT("[SendUseSkill] SkillId=%d ChargeScale=%f TargetId=%llu"),
		SkillId,
		CurrentFireballChargeScale,
		(uint64)pkt.targetid());
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
	SendEnterGamePacket();
}

void UMainGameInstance::HandleDamage(const Protocol::S_DAMAGE_PLAYER& pkt)
{
	uint64 ObjectId = pkt.object_id();
	float Damage = pkt.damage();

	UE_LOG(LogTemp, Warning, TEXT("[HandleDamage] ObjId=%llu Damage=%f"), ObjectId, Damage);

	TWeakObjectPtr<APlayerCharacter>* FoundActor = Players.Find(ObjectId);
	if (FoundActor == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HandleDamage] Player not found in map"));
		return;
	}

	APlayerCharacter* Actor = FoundActor->Get();
	if (Actor == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HandleDamage] Invalid actor, remove stale entry"));
		Players.Remove(ObjectId);
		return;
	}
	Actor->AttributeComponent->SubtractHealth(Damage);
	Actor->PlayOtherPlayerSkill(0);
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

	APlayerCharacter* Player = GetPlayerById(PlayerId);
	if (Player == nullptr)
		return;

	if (!Player->IsMyPlayer())
	{
		Player->PlayOtherPlayerSkill(SkillId);
	}

	switch (SkillId)
	{
	case 0:
		HandleIceSkillPacket(PlayerId, TargetId);
		break;

	case 1:
		HandleFireballSkillPacket(PlayerId, ChargeScale);
		break;

	default:
		break;
	}
	UE_LOG(LogTemp, Warning, TEXT("[OnRecvUseSkill] PlayerId=%llu MyObjectId=%llu SkillId=%d IsMine=%d"),
		PlayerId,
		MyObjectId,
		SkillId,
		Player->IsMyPlayer() ? 1 : 0);
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

	UE_LOG(LogTemp, Warning, TEXT("[IceCombo] UseHand=%s NextHand=%s CasterID=%llu Time=%f SpawnLocation=%s"),
		bUseRightHand ? TEXT("Right") : TEXT("Left"),
		(!bUseRightHand) ? TEXT("Right") : TEXT("Left"),
		(unsigned long long)CasterID,
		CurrentTime,
		*SpawnLocation.ToString());

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
		Projectile->SetProjectileInfo(Caster, 0);

		UE_LOG(LogTemp, Warning, TEXT("[HandleIceSkillPacket] %s SetProjectileInfo Owner=%s SkillId=%d"),
			HandName,
			*GetNameSafe(Caster),
			0);
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
		return;

	if (FireballProjectileBPClass == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("FireballProjectileBPClass is null"));
		return;
	}

	ChargeScale = FMath::Clamp(ChargeScale, 0.2f, 2.0f);

	FVector Forward = Caster->GetActorForwardVector();
	FVector SpawnLocation = Caster->GetMesh()->GetSocketLocation(TEXT("RightHandSpellSocket")) + Forward * 50.f;
	FRotator SpawnRotation = Forward.Rotation();

	AProjectile* Projectile = GetWorld()->SpawnActor<AProjectile>(
		FireballProjectileBPClass,
		SpawnLocation,
		SpawnRotation
	);

	if (Projectile == nullptr)
		return;

	Projectile->SetOwner(Caster);
	Projectile->SetProjectileInfo(Caster, 1);
	Projectile->SetChargeScale(ChargeScale);
	Projectile->ActivateProjectileCollision();
	Projectile->LaunchProjectile(Forward);
}
