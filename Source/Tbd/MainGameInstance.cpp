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
		StartServerProcess();
		FCoreDelegates::OnPreExit.AddUObject(this, &UMainGameInstance::StopServerProcess);
		UE_LOG(LogTemp, Warning, TEXT("[MainGameInstance::Init] Server process started (NOT clientonly)"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[MainGameInstance::Init] clientonly: skip StartServerProcess"));
	}

	ClientPacketHandler::Init();

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

void UMainGameInstance::HandleSpawn(const Protocol::PlayerInfo& PlayerInfo, bool IsMine)
{
	if (Socket == nullptr || GameServerSession == nullptr)
		return;

	auto* World = GetWorld();
	if (World == nullptr)
		return;

	const uint64 ObjectId = PlayerInfo.object_id();
	
	// 이미 존재하는 플레이어가 있으면 제거하고 다시 스폰
	APlayerCharacter** ExistingPlayer = Players.Find(ObjectId);
	if (ExistingPlayer != nullptr && *ExistingPlayer != nullptr)
	{
		// MyPlayer가 아니면 제거
		if (*ExistingPlayer != MyPlayer)
		{
			World->DestroyActor(*ExistingPlayer);
		}
		Players.Remove(ObjectId);
	}

	FVector SpawnLocation(PlayerInfo.x(), PlayerInfo.y(), PlayerInfo.z());
	UE_LOG(LogTemp, Warning, TEXT("Spawn Pos: %f %f %f"),PlayerInfo.x(), PlayerInfo.y(), PlayerInfo.z());

	UE_LOG(LogTemp, Warning, TEXT("HandleSpawn: id=%llu IsMine=%d"), PlayerInfo.object_id(), IsMine);

	if (IsMine)
	{
		auto PC = UGameplayStatics::GetPlayerController(this, 0);
		APlayerCharacter* Player = Cast<APlayerCharacter>(PC->GetPawn());
		if (Player == nullptr)
			return;

		Player->bIsMine = true;
		Player->SetPlayerInfo(PlayerInfo);

		MyPlayer = Player;
		Players.Add(PlayerInfo.object_id(), Player);

		if (UC_NetworkPlayerComponent* NetComp = Player->FindComponentByClass<UC_NetworkPlayerComponent>())
		{
			NetComp->SetObjectId(PlayerInfo.object_id());
			UE_LOG(LogTemp, Warning, TEXT("My ObjectId Set: %llu"), PlayerInfo.object_id());
		}
	}

	else
	{
		// 충돌 방지를 위해 약간 옆으로 오프셋
		SpawnLocation += FVector(30.f, 0.f, 0.f);
		FRotator SpawnRotation(0.f, PlayerInfo.yaw(), 0.f);

		APlayerCharacter* Player = World->SpawnActor<APlayerCharacter>(
			OtherPlayerClass,
			SpawnLocation,
			SpawnRotation
		);

		UE_LOG(LogTemp, Warning, TEXT("SpawnActor Success? %s"),
			Player ? TEXT("YES") : TEXT("NO"));

		if (Player == nullptr)
			return;

		Player->bIsMine = false;
		Player->SetPlayerInfo(PlayerInfo);

		Players.Add(PlayerInfo.object_id(), Player);
	}
}

void UMainGameInstance::HandleSpawn(const Protocol::S_ENTER_GAME& EnterGamePkt)
{
	MyObjectId = EnterGamePkt.player().object_id();
	HandleSpawn(EnterGamePkt.player(), true);
}

void UMainGameInstance::HandleSpawn(const Protocol::S_SPAWN& SpawnPkt)
{
	for (auto& Player : SpawnPkt.players())
	{
		const bool bIsMine = (Player.object_id() == MyObjectId);
		HandleSpawn(Player, bIsMine);
	}
}

void UMainGameInstance::HandleDespawn(uint64 ObjectId)
{
	if (Socket == nullptr || GameServerSession == nullptr)
		return;

	auto* World = GetWorld();
	if (World == nullptr)
		return;

	APlayerCharacter** FindActor = Players.Find(ObjectId);
	if (FindActor == nullptr)
		return;

	APlayerCharacter* PlayerToDestroy = *FindActor;
	
	// Players 맵에서 제거
	Players.Remove(ObjectId);
	
	// MyPlayer인 경우 nullptr로 설정
	if (MyPlayer == PlayerToDestroy)
	{
		MyPlayer = nullptr;
	}

	// 액터 파괴
	if (PlayerToDestroy != nullptr)
	{
		World->DestroyActor(PlayerToDestroy);
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


	Player->SetDestInfo(MovePkt.info());
	//const Protocol::PlayerInfo& Info = MovePkt.info();
	//Player->SetPlayerInfo(Info);
}

void UMainGameInstance::HandleSpawnMob(const Protocol::S_SPAWN_MOB& Pkt)
{
	UE_LOG(LogTemp, Warning, TEXT("[HandleSpawnMob] ===== FUNCTION CALLED ====="));
	UE_LOG(LogTemp, Warning, TEXT("[HandleSpawnMob] Received packet with %d mobs"), Pkt.mobs_size());
	
	auto* World = GetWorld();
	if (World == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[HandleSpawnMob] World is nullptr!"));
		return;
	}
	UE_LOG(LogTemp, Warning, TEXT("[HandleSpawnMob] World is valid"));

	if (MonsterClass == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[HandleSpawnMob] MonsterClass is nullptr! Check Blueprint settings."));
		return;
	}
	UE_LOG(LogTemp, Warning, TEXT("[HandleSpawnMob] MonsterClass is valid: %s"), 
		MonsterClass ? *MonsterClass->GetName() : TEXT("nullptr"));

	UE_LOG(LogTemp, Warning, TEXT("[HandleSpawnMob] Processing %d mobs"), Pkt.mobs_size());

	int32 SuccessCount = 0;
	int32 FailCount = 0;

	for (auto& MobInfo : Pkt.mobs())
	{
		const uint64 MobId = MobInfo.mobid();

		UE_LOG(LogTemp, Warning, TEXT("[HandleSpawnMob] Attempting to spawn Mob ID=%llu"), MobId);

		// 기존 몬스터 제거
		if (AActor** Existing = Monsters.Find(MobId))
		{
			if (*Existing != nullptr)
				World->DestroyActor(*Existing);
			Monsters.Remove(MobId);
		}

		FVector SpawnPos(MobInfo.pos().x(), MobInfo.pos().y(), MobInfo.pos().z());
		FRotator SpawnRot(0.f, 0.f, 0.f);

		UE_LOG(LogTemp, Warning, TEXT("[HandleSpawnMob] Server Spawn Pos: [X=%.3f Y=%.3f Z=%.3f]"), 
			SpawnPos.X, SpawnPos.Y, SpawnPos.Z);

		// 서버에서 보낸 정확한 위치 사용 (오프셋 제거)
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		AActor* MobActor = World->SpawnActor<AActor>(
			MonsterClass,
			SpawnPos,
			SpawnRot,
			SpawnParams
		);

		if (MobActor)
		{
			if (ACharacter* Character = Cast<ACharacter>(MobActor))
			{
				if (UCharacterMovementComponent* MovementComp = Character->GetCharacterMovement())
				{
					// 지면 레이캐스트로 지면 위치 찾기
					FHitResult HitResult;
					FVector TraceStart = SpawnPos + FVector(0, 0, 1000.f);
					FVector TraceEnd = SpawnPos + FVector(0, 0, -10000.f);
					
					FVector GroundPos = SpawnPos;
					if (World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_WorldStatic))
					{
						GroundPos = HitResult.ImpactPoint;
						GroundPos.Z += 100.f; // 캡슐 높이 고려
						UE_LOG(LogTemp, Warning, TEXT("[HandleSpawnMob] Line trace hit: ImpactPoint Z=%.3f, Adjusted Z=%.3f"), 
							HitResult.ImpactPoint.Z, GroundPos.Z);
					}
					else
					{
						UE_LOG(LogTemp, Warning, TEXT("[HandleSpawnMob] Line trace failed, using SpawnPos"));
					}
					
					// 중력 강제 활성화 (여러 번 설정)
					MovementComp->GravityScale = 1.0f;
					MovementComp->bApplyGravityWhileJumping = true;
					MovementComp->bCanWalkOffLedges = true;
					MovementComp->bUseControllerDesiredRotation = false;
					
					// 물리 시뮬레이션 활성화 확인
					Character->SetActorEnableCollision(true);
					
					// Falling 모드로 설정하여 중력 강제 적용
					MovementComp->SetMovementMode(MOVE_Falling);
					MovementComp->Velocity = FVector::ZeroVector; // 속도 초기화
					MovementComp->GravityScale = 1.0f;
					
					// 위치를 여러 번 강제 설정 (블루프린트 BeginPlay가 덮어쓸 수 있으므로)
					if (USceneComponent* RootComp = Character->GetRootComponent())
					{
						RootComp->SetWorldLocation(GroundPos, false, nullptr, ETeleportType::TeleportPhysics);
						RootComp->SetWorldLocation(GroundPos, false, nullptr, ETeleportType::TeleportPhysics); // 두 번 호출
					}
					
					Character->SetActorLocation(GroundPos, false, nullptr, ETeleportType::TeleportPhysics);
					Character->SetActorLocation(GroundPos, false, nullptr, ETeleportType::TeleportPhysics); // 두 번 호출
					
					// 중력 다시 설정
					MovementComp->GravityScale = 1.0f;
					MovementComp->SetMovementMode(MOVE_Walking);
					MovementComp->Velocity = FVector::ZeroVector; // 속도 다시 초기화
					
					UE_LOG(LogTemp, Warning, TEXT("[HandleSpawnMob] Initial Setup: GroundPos [X=%.3f Y=%.3f Z=%.3f], GravityScale=%.2f, MovementMode=%d"), 
						GroundPos.X, GroundPos.Y, GroundPos.Z, MovementComp->GravityScale, (int32)MovementComp->MovementMode);
					
					// BeginPlay 이후에 다시 한번 확실히 설정 (블루프린트 BeginPlay가 실행된 후)
					World->GetTimerManager().SetTimerForNextTick([Character, MovementComp, World, GroundPos]()
					{
						UE_LOG(LogTemp, Warning, TEXT("[HandleSpawnMob] Timer Callback 1: Checking validity..."));
						
						if (!IsValid(Character))
						{
							UE_LOG(LogTemp, Error, TEXT("[HandleSpawnMob] Timer Callback 1: Character is invalid!"));
							return;
						}
						
						if (!IsValid(MovementComp))
						{
							UE_LOG(LogTemp, Error, TEXT("[HandleSpawnMob] Timer Callback 1: MovementComp is invalid!"));
							return;
						}
						
						if (!IsValid(World))
						{
							UE_LOG(LogTemp, Error, TEXT("[HandleSpawnMob] Timer Callback 1: World is invalid!"));
							return;
						}
						
						UE_LOG(LogTemp, Warning, TEXT("[HandleSpawnMob] Timer Callback 1: All valid, applying settings..."));
						
						// 중력 다시 한번 확실히 설정
						MovementComp->GravityScale = 1.0f;
						MovementComp->bApplyGravityWhileJumping = true;
						MovementComp->Velocity = FVector::ZeroVector; // 속도 초기화
						
						// 지면 레이캐스트로 정확한 지면 위치 찾기
						FHitResult HitResult;
						FVector CurrentPos = Character->GetActorLocation();
						FVector TraceStart = CurrentPos + FVector(0, 0, 500.f);
						FVector TraceEnd = CurrentPos + FVector(0, 0, -10000.f);
						
						UE_LOG(LogTemp, Warning, TEXT("[HandleSpawnMob] Timer Callback 1: Current Pos [X=%.3f Y=%.3f Z=%.3f]"), 
							CurrentPos.X, CurrentPos.Y, CurrentPos.Z);
						
						FVector FinalGroundPos = GroundPos;
						if (World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_WorldStatic))
						{
							FinalGroundPos = HitResult.ImpactPoint;
							FinalGroundPos.Z += 100.f;
							UE_LOG(LogTemp, Warning, TEXT("[HandleSpawnMob] Timer Callback 1: Line trace hit, FinalGroundPos Z=%.3f"), FinalGroundPos.Z);
						}
						else
						{
							UE_LOG(LogTemp, Warning, TEXT("[HandleSpawnMob] Timer Callback 1: Line trace failed, using GroundPos Z=%.3f"), GroundPos.Z);
						}

						// RootComponent와 Actor 위치 모두 강제 설정 (여러 번 호출)
						if (USceneComponent* RootComp = Character->GetRootComponent())
						{
							RootComp->SetWorldLocation(FinalGroundPos, false, nullptr, ETeleportType::TeleportPhysics);
							RootComp->SetWorldLocation(FinalGroundPos, false, nullptr, ETeleportType::TeleportPhysics);
						}
						Character->SetActorLocation(FinalGroundPos, false, nullptr, ETeleportType::TeleportPhysics);
						Character->SetActorLocation(FinalGroundPos, false, nullptr, ETeleportType::TeleportPhysics);
						
						// Falling으로 설정했다가 Walking으로 변경 (중력 강제 적용)
						MovementComp->SetMovementMode(MOVE_Falling);
						MovementComp->GravityScale = 1.0f;
						MovementComp->Velocity = FVector::ZeroVector;
						MovementComp->SetMovementMode(MOVE_Walking);
						
						// 실제 위치 확인
						FVector ActualPos = Character->GetActorLocation();
						UE_LOG(LogTemp, Warning, TEXT("[HandleSpawnMob] Post-BeginPlay: Target [X=%.3f Y=%.3f Z=%.3f], Actual [X=%.3f Y=%.3f Z=%.3f], GravityScale=%.2f, MovementMode=%d"), 
							FinalGroundPos.X, FinalGroundPos.Y, FinalGroundPos.Z,
							ActualPos.X, ActualPos.Y, ActualPos.Z,
							MovementComp->GravityScale, (int32)MovementComp->MovementMode);

						// 한 틱 더 기다렸다가 최종 확인
						World->GetTimerManager().SetTimerForNextTick([Character, MovementComp, World]()
						{
							UE_LOG(LogTemp, Warning, TEXT("[HandleSpawnMob] Timer Callback 2: Final check..."));
							
							if (!IsValid(Character))
							{
								UE_LOG(LogTemp, Error, TEXT("[HandleSpawnMob] Timer Callback 2: Character is invalid!"));
								return;
							}
							
							if (!IsValid(MovementComp))
							{
								UE_LOG(LogTemp, Error, TEXT("[HandleSpawnMob] Timer Callback 2: MovementComp is invalid!"));
								return;
							}
							
							// 최종 중력 및 위치 확인 및 강제 설정
							MovementComp->GravityScale = 1.0f;
							MovementComp->Velocity = FVector::ZeroVector;
							MovementComp->SetMovementMode(MOVE_Walking);
							
							// 지면 레이캐스트로 최종 확인
							FHitResult HitResult;
							FVector CurrentPos = Character->GetActorLocation();
							FVector TraceStart = CurrentPos + FVector(0, 0, 500.f);
							FVector TraceEnd = CurrentPos + FVector(0, 0, -10000.f);
							
							if (World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_WorldStatic))
							{
								FVector FinalPos = HitResult.ImpactPoint;
								FinalPos.Z += 100.f;
								
								// 위치가 많이 벗어나면 다시 설정
								if (FMath::Abs(CurrentPos.Z - FinalPos.Z) > 50.f)
								{
									if (USceneComponent* RootComp = Character->GetRootComponent())
									{
										RootComp->SetWorldLocation(FinalPos, false, nullptr, ETeleportType::TeleportPhysics);
									}
									Character->SetActorLocation(FinalPos, false, nullptr, ETeleportType::TeleportPhysics);
									UE_LOG(LogTemp, Warning, TEXT("[HandleSpawnMob] Final Check: Position corrected from Z=%.3f to Z=%.3f"), 
										CurrentPos.Z, FinalPos.Z);
								}
							}
							
							FVector FinalPos = Character->GetActorLocation();
							UE_LOG(LogTemp, Warning, TEXT("[HandleSpawnMob] Final Check: GravityScale=%.2f, MovementMode=%d, Location [X=%.3f Y=%.3f Z=%.3f], Velocity [X=%.3f Y=%.3f Z=%.3f]"), 
								MovementComp->GravityScale, (int32)MovementComp->MovementMode, 
								FinalPos.X, FinalPos.Y, FinalPos.Z,
								MovementComp->Velocity.X, MovementComp->Velocity.Y, MovementComp->Velocity.Z);
						});
					});
					
					UE_LOG(LogTemp, Warning, TEXT("[HandleSpawnMob] Character Movement Setup: GravityScale=%.2f, Ground Pos: [X=%.3f Y=%.3f Z=%.3f]"), 
						MovementComp->GravityScale, GroundPos.X, GroundPos.Y, GroundPos.Z);
				}
			}

			Monsters.Add(MobId, MobActor);
			SuccessCount++;
			
			// 실제 액터 위치 확인
			FVector ActualPos = MobActor->GetActorLocation();
			UE_LOG(LogTemp, Warning, TEXT("[HandleSpawnMob] SUCCESS - Mob Spawned: ID=%llu, Server Pos [X=%.3f Y=%.3f Z=%.3f], Actual Pos [X=%.3f Y=%.3f Z=%.3f]"), 
				MobId, SpawnPos.X, SpawnPos.Y, SpawnPos.Z, ActualPos.X, ActualPos.Y, ActualPos.Z);
		}
		else
		{
			FailCount++;
			UE_LOG(LogTemp, Error, TEXT("[HandleSpawnMob] FAILED - Mob Spawn FAILED: ID=%llu at [X=%.3f Y=%.3f Z=%.3f] (MonsterClass: %s)"), 
				MobId, SpawnPos.X, SpawnPos.Y, SpawnPos.Z, 
				MonsterClass ? *MonsterClass->GetName() : TEXT("NULL"));
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[HandleSpawnMob] Summary: Total=%d, Success=%d, Failed=%d"), 
		Pkt.mobs_size(), SuccessCount, FailCount);
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

static constexpr int32 ICE_SKILL_ID = 0;
static constexpr int32 FIREBALL_SKILL_ID = 1;

void UMainGameInstance::OnRecvUseSkill(const Protocol::S_USE_SKILL& pkt)
{
	if (GWorld == nullptr) return;

	const int64 PlayerId = (int64)pkt.playerid();
	const int32 SkillId = (int32)pkt.skillid();

	FVector SpawnPos(pkt.spawnpos().x(), pkt.spawnpos().y(), pkt.spawnpos().z());
	FVector Dir(pkt.dir().x(), pkt.dir().y(), pkt.dir().z());
	Dir = Dir.GetSafeNormal();

	const int32 ClientShotId = pkt.clientshotid();
	const int32 ProjectileId = pkt.projectileid();

	const bool bIsMine = (PlayerId == MyObjectId);

	if (!bIsMine)
	{
		APlayerCharacter** Found = Players.Find(PlayerId);
		if (Found == nullptr || *Found == nullptr) return;
		(*Found)->PlayOtherPlayerSkill(SkillId);
	}

	if (bIsMine)
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
	else
		return;

	if (!ProjClass)
	{
		UE_LOG(LogTemp, Error, TEXT("[OnRecvUseSkill] Projectile BP class is null for SkillId=%d"), SkillId);
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AActor* NewProj = GWorld->SpawnActor<AActor>(ProjClass, SpawnPos, Dir.Rotation(), Params);

	if (NewProj && ProjectileId != 0)
		ByProjectileId.Add(ProjectileId, NewProj);
}
