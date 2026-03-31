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
	{
		UE_LOG(LogTemp, Error, TEXT("[HandleSpawn] World nullptr IsMine=%d ObjId=%llu"), IsMine ? 1 : 0, PlayerInfo.object_id());
		return;
	}

	const uint64 ObjectId = PlayerInfo.object_id();
	FVector SpawnLocation(PlayerInfo.x(), PlayerInfo.y(), PlayerInfo.z());
	FRotator SpawnRotation(0.f, PlayerInfo.yaw(), 0.f);

	UE_LOG(LogTemp, Warning, TEXT("[HandleSpawn] Enter IsMine=%d ObjId=%llu World=%s"),
		IsMine ? 1 : 0, ObjectId, *GetNameSafe(World));

	if (IsMine)
	{
		if (MyPlayer != nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("[HandleSpawn-Mine] Already linked. Skip ObjId=%llu CurrentMyObjId=%llu Pawn=%s"),
				ObjectId, MyObjectId, *GetNameSafe(MyPlayer));
			return;
		}

		APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
		if (!PC)
		{
			UE_LOG(LogTemp, Error, TEXT("[HandleSpawn-Mine] PC nullptr ObjId=%llu"), ObjectId);
			return;
		}

		UE_LOG(LogTemp, Warning, TEXT("[HandleSpawn-Mine] PC=%s"), *GetNameSafe(PC));

		APawn* Pawn = PC->GetPawn();
		if (!Pawn)
		{
			UE_LOG(LogTemp, Error, TEXT("[HandleSpawn-Mine] Pawn nullptr ObjId=%llu"), ObjectId);
			return;
		}

		UE_LOG(LogTemp, Warning, TEXT("[HandleSpawn-Mine] Pawn=%s"), *GetNameSafe(Pawn));

		APlayerCharacter* LocalPlayer = Cast<APlayerCharacter>(Pawn);
		if (!LocalPlayer)
		{
			UE_LOG(LogTemp, Error, TEXT("[HandleSpawn-Mine] Cast failed Pawn=%s Class=%s ObjId=%llu"),
				*GetNameSafe(Pawn),
				*GetNameSafe(Pawn->GetClass()),
				ObjectId);
			return;
		}

		LocalPlayer->bIsMine = true;
		LocalPlayer->SetPlayerInfo(PlayerInfo);

		LocalPlayer->SetActorLocation(SpawnLocation);
		LocalPlayer->SetActorRotation(SpawnRotation);

		UE_LOG(LogTemp, Warning, TEXT("[HandleSpawn-Mine] ServerPos=(%.1f, %.1f, %.1f) CurrentPawnPos=(%.1f, %.1f, %.1f)"),
			SpawnLocation.X, SpawnLocation.Y, SpawnLocation.Z,
			LocalPlayer->GetActorLocation().X,
			LocalPlayer->GetActorLocation().Y,
			LocalPlayer->GetActorLocation().Z);

		if (UCharacterMovementComponent* MoveComp = LocalPlayer->GetCharacterMovement())
		{
			MoveComp->StopMovementImmediately();
		}

		MyPlayer = LocalPlayer;
		MyObjectId = ObjectId;
		Players.FindOrAdd(ObjectId) = LocalPlayer;

		UE_LOG(LogTemp, Warning, TEXT("[HandleSpawn-Mine] SUCCESS ObjId=%llu Pawn=%s"),
			ObjectId, *GetNameSafe(LocalPlayer));
		return;
	}

	APlayerCharacter* TargetActor = Players.Contains(ObjectId) ? Players[ObjectId] : nullptr;

	if (IsValid(TargetActor))
	{
		UE_LOG(LogTemp, Warning, TEXT("[HandleSpawn-Other] Update existing ObjId=%llu Actor=%s"),
			ObjectId, *GetNameSafe(TargetActor));
		TargetActor->SetPlayerInfo(PlayerInfo);
	}
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

			UE_LOG(LogTemp, Warning, TEXT("[SpawnOther] Spawned ObjId=%llu Actor=%s"),
				ObjectId, *NewOther->GetName());
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[HandleSpawn-Other] Spawn failed ObjId=%llu"), ObjectId);
		}
	}
}

void UMainGameInstance::HandleSpawn(const Protocol::S_ENTER_GAME& EnterGamePkt)
{
	UE_LOG(LogTemp, Warning, TEXT("[GI::HandleSpawn(S_ENTER_GAME)] Before MyObjectId=%llu IncomingObjId=%llu"),
		MyObjectId,
		static_cast<unsigned long long>(EnterGamePkt.player().object_id()));

	if (MyObjectId != 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GI::HandleSpawn(S_ENTER_GAME)] Skip: MyObjectId already set"));
		return;
	}

	MyObjectId = EnterGamePkt.player().object_id();

	UE_LOG(LogTemp, Warning, TEXT("[GI::HandleSpawn(S_ENTER_GAME)] Set MyObjectId=%llu"),
		MyObjectId);

	HandleSpawn(EnterGamePkt.player(), true);
}

void UMainGameInstance::HandleSpawn(const Protocol::S_SPAWN& SpawnPkt)
{
	UE_LOG(LogTemp, Warning, TEXT("[S_SPAWN] players_size=%d"), SpawnPkt.players_size());

	for (const auto& PlayerInfo : SpawnPkt.players())
	{
		uint64 ObjectId = PlayerInfo.object_id();

		UE_LOG(LogTemp, Warning, TEXT("[S_SPAWN] incoming objId=%llu myObjId=%llu"),
			ObjectId, MyObjectId);

		if (ObjectId == MyObjectId)
		{
			UE_LOG(LogTemp, Log, TEXT("Skip S_SPAWN for MySelf ID: %llu"), ObjectId);
			continue;
		}

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
	UE_LOG(LogTemp, Warning, TEXT("[HandleMove] Enter"));

	if (Socket == nullptr || GameServerSession == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[HandleMove] Socket or GameServerSession is nullptr"));
		return;
	}

	auto* World = GetWorld();
	if (World == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[HandleMove] World is nullptr"));
		return;
	}

	const uint64 ObjectId = MovePkt.info().object_id();
	UE_LOG(LogTemp, Warning, TEXT("[HandleMove] ObjId=%llu"), ObjectId);

	APlayerCharacter** FindActor = Players.Find(ObjectId);
	if (FindActor == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[HandleMove] Players.Find failed for ObjId=%llu"), ObjectId);
		return;
	}

	APlayerCharacter* Player = (*FindActor);
	if (Player == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[HandleMove] Player is nullptr for ObjId=%llu"), ObjectId);
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[HandleMove] Apply move ObjId=%llu Actor=%s"),
		ObjectId, *GetNameSafe(Player));

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
	if (MyObjectId == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Client] SendUseSkill failed: MyPlayerId is 0"));
		return;
	}

	UE_LOG(LogTemp, Warning,TEXT("[Client] Send C_USE_SKILL player=%llu skill=%d"), MyObjectId, SkillId);

	Protocol::C_USE_SKILL pkt;
	pkt.set_playerid(MyObjectId);
	pkt.set_skillid(SkillId);

	SendPacket(ClientPacketHandler::MakeSendBuffer(pkt));
}

void UMainGameInstance::SendEnterGamePacket()
{
	UE_LOG(LogTemp, Warning, TEXT("=== SendEnterGamePacket Start ==="));

	if (!GameServerSession.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Critical Error: GameServerSession is NOT Valid! Connection might be lost."));
		return;
	}

	Protocol::C_ENTER_GAME EnterGamePkt;
	auto SendBuffer = ClientPacketHandler::MakeSendBuffer(EnterGamePkt);

	GameServerSession->SendPacket(SendBuffer);
	UE_LOG(LogTemp, Log, TEXT("Successfully Sent C_ENTER_GAME Packet"));
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