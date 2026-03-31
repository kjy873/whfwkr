// Fill out your copyright notice in the Description page of Project Settings.


#include "MainGameInstance.h"
#include "Sockets.h"
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

	// 이전 연결이 있으면 정리
	if (GameServerSession.IsValid())
	{
		DisconnectToGameServer();
		GameServerSession.Reset();
	}

	// 이전 플레이어들 정리
	auto* World = GetWorld();
	if (World != nullptr)
	{
		for (auto& Pair : Players)
		{
			if (Pair.Value != nullptr && Pair.Value != MyPlayer)
			{
				World->DestroyActor(Pair.Value);
			}
		}
	}
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
		//StartServerProcess();
		FCoreDelegates::OnPreExit.AddUObject(this, &UMainGameInstance::StopServerProcess);
		UE_LOG(LogTemp, Warning, TEXT("[MainGameInstance::Init] Server process started (NOT clientonly)"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[MainGameInstance::Init] clientonly: skip StartServerProcess"));
	}

	ClientPacketHandler::Init();

	ConnectToGameServer();

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
	if (MyPlayer && MyPlayer->PlayerInfo.object_id() == PlayerId)
	{
		return MyPlayer;
	}

	for (auto& Pair : Players)
	{
		APlayerCharacter* Player = Pair.Value;
		if (Player && Player->PlayerInfo.object_id() == PlayerId)
		{
			return Player;
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

	UE_LOG(LogTemp, Warning, TEXT("Sent C_ENTER_GAME as Level Ready signal"));
}

void UMainGameInstance::HandleSpawn(const Protocol::PlayerInfo& PlayerInfo, bool IsMine)
{
	UWorld* World = GetWorld();
	if (!World)
		return;

	const uint64 ObjectId = PlayerInfo.object_id();
	FVector SpawnLocation(PlayerInfo.x(), PlayerInfo.y(), PlayerInfo.z());
	FRotator SpawnRotation(0.f, PlayerInfo.yaw(), 0.f);


	if (IsMine)
	{
		if (MyPlayer != nullptr)
			return;

		APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
		if (!PC)
			return;

		APawn* Pawn = PC->GetPawn();
		if (!Pawn)
			return;

		APlayerCharacter* LocalPlayer = Cast<APlayerCharacter>(Pawn);
		if (!LocalPlayer)
			return;

		LocalPlayer->bIsMine = true;
		LocalPlayer->SetPlayerInfo(PlayerInfo);

		LocalPlayer->SetActorLocation(SpawnLocation);
		LocalPlayer->SetActorRotation(SpawnRotation);

		if (UCharacterMovementComponent* MoveComp = LocalPlayer->GetCharacterMovement())
		{
			MoveComp->StopMovementImmediately();
		}

		MyPlayer = LocalPlayer;
		MyObjectId = ObjectId;
		Players.FindOrAdd(ObjectId) = LocalPlayer;

		return;
	}

	APlayerCharacter* TargetActor = Players.Contains(ObjectId) ? Players[ObjectId] : nullptr;

	if (IsValid(TargetActor))
		TargetActor->SetPlayerInfo(PlayerInfo);

	else if (OtherPlayerClass && !Players.Contains(ObjectId))
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		APlayerCharacter* NewOther = World->SpawnActor<APlayerCharacter>(OtherPlayerClass, SpawnLocation, SpawnRotation, SpawnParams);
		if (NewOther)
		{
			NewOther->bIsMine = false;
			NewOther->SetPlayerInfo(PlayerInfo);
			Players.FindOrAdd(ObjectId) = NewOther;
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
		uint64 ObjectId = PlayerInfo.object_id();

		if (ObjectId == MyObjectId)
			continue;

		HandleSpawn(PlayerInfo, false);
	}
}

void UMainGameInstance::HandleDespawn(uint64 ObjectId)
{
	if (Socket == nullptr || GameServerSession == nullptr) return;
	UWorld* World = GetWorld();
	if (World == nullptr) return;

	APlayerCharacter** FindActor = Players.Find(ObjectId);
	if (FindActor == nullptr || *FindActor == nullptr) return;

	APlayerCharacter* PlayerToDestroy = *FindActor;
	Players.Remove(ObjectId);

	if (MyPlayer == PlayerToDestroy) return;

	if (IsValid(PlayerToDestroy))
	{
		PlayerToDestroy->Destroy();
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

	APlayerCharacter** FindActor = Players.Find(ObjectId);
	if (FindActor == nullptr)
		return;

	APlayerCharacter* Player = (*FindActor);
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
	UE_LOG(LogTemp, Warning, TEXT("[Client] Send C_USE_SKILL skillId=%d"), SkillId);

	if (MyObjectId == 0)
		return;

	Protocol::C_USE_SKILL pkt;
	pkt.set_playerid(MyObjectId);
	pkt.set_skillid(SkillId);

	SendPacket(ClientPacketHandler::MakeSendBuffer(pkt));
}

void UMainGameInstance::SendEnterGamePacket()
{
	if (!GameServerSession.IsValid())
		return;

	Protocol::C_ENTER_GAME EnterGamePkt;
	auto SendBuffer = ClientPacketHandler::MakeSendBuffer(EnterGamePkt);

	GameServerSession->SendPacket(SendBuffer);
}

void UMainGameInstance::HandleDamage(const Protocol::S_DAMAGE_PLAYER& pkt)
{
	uint64 ObjectId = pkt.object_id();
	float Damage = pkt.damage();

	UE_LOG(LogTemp, Warning, TEXT("[HandleDamage] ObjId=%llu Damage=%f"), ObjectId, Damage);

	APlayerCharacter** FoundActor = Players.Find(ObjectId);
	if (FoundActor == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HandleDamage] Player not found"));
		return;
	}

	APlayerCharacter* Target = *FoundActor;
	if (Target == nullptr)
		return;

	Target->SubtractHealth(Damage);

	Target->PlayOtherPlayerSkill(0);
}

void UMainGameInstance::HandleDie(const Protocol::S_PLAYER_DEAD& Pkt)
{
	uint64 ObjectId = Pkt.object_id();

	UE_LOG(LogTemp, Warning, TEXT("[HandleDie] this=%p ObjId=%llu Players.Num=%d"),
		this, ObjectId, Players.Num());

	APlayerCharacter** FoundActor = Players.Find(ObjectId);
	if (FoundActor == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HandleDie] Player not found. ObjId=%llu"), ObjectId);
		return;
	}

	APlayerCharacter* TargetCharacter = *FoundActor;
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
	Protocol::C_ATTACK_PLAYER pkt;
	pkt.set_targetplayerid(TargetId);
	pkt.set_skillid(SkillId);

	SendPacket(ClientPacketHandler::MakeSendBuffer(pkt));

	UE_LOG(LogTemp, Warning, TEXT("[Client] SendAttackPlayer target=%llu skill=%d"),
		TargetId, SkillId);
}

static constexpr int32 ICE_SKILL_ID = 0;
static constexpr int32 FIREBALL_SKILL_ID = 1;

void UMainGameInstance::OnRecvUseSkill(const Protocol::S_USE_SKILL& pkt)
{
	if (GWorld == nullptr) return;

	const int64 PlayerId = (int64)pkt.playerid();
	const int32 SkillId = (int32)pkt.skillid();
	const int32 ClientShotId = pkt.clientshotid();
	const int32 ProjectileId = pkt.projectileid();
 
	const bool bIsMine = (PlayerId == MyObjectId);

	FVector SpawnPos(pkt.spawnpos().x(), pkt.spawnpos().y(), pkt.spawnpos().z());
	FVector Dir(pkt.dir().x(), pkt.dir().y(), pkt.dir().z());
	Dir = Dir.GetSafeNormal();

	if (!bIsMine)
	{
		APlayerCharacter** Found = Players.Find(PlayerId);
		if (Found && *Found)
		{
			(*Found)->PlayOtherPlayerSkill(SkillId);
		}

	}
	else
	{
		if (AActor** PredPtr = PredictedByShotId.Find(ClientShotId))
		{
			AActor* Pred = *PredPtr;
			if (Pred)
			{
				Pred->SetActorLocation(SpawnPos);
				Pred->SetActorRotation(Dir.Rotation());

				if (ProjectileId != 0)
					ByProjectileId.Add(ProjectileId, Pred);
			}

			PredictedByShotId.Remove(ClientShotId);
			return;
		}
	}

	TSubclassOf<AActor> ProjClass = nullptr;
	if (SkillId == ICE_SKILL_ID)
		ProjClass = IceProjectileBPClass;
	else if (SkillId == FIREBALL_SKILL_ID)
		ProjClass = FireballProjectileBPClass;

	if (!ProjClass) return;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AActor* NewProj = GWorld->SpawnActor<AActor>(ProjClass, SpawnPos, Dir.Rotation(), Params);

	if (NewProj && ProjectileId != 0)
		ByProjectileId.Add(ProjectileId, NewProj);
}