// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Tbd.h"
#include "Protocol.pb.h"
#include "MainGameInstance.generated.h"

class AActor;
class APlayerCharacter;
/**
 * 
 */
UCLASS()
class TBD_API UMainGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void ConnectToGameServer();

	UFUNCTION(BlueprintCallable)
	void DisconnectToGameServer();

	UFUNCTION(BlueprintCallable)
	void HandleRecvPackets();

	UFUNCTION(BlueprintCallable, Category = "Network")
	void SendAttackMob(int64 MobId);

	UFUNCTION(BlueprintCallable)
	void SendUseSkill(int32 SkillId);

	void OnRecvUseSkill(int64 PlayerId, int32 SkillId);

	virtual void Init() override;

	UPROPERTY()
	uint64 MyObjectId = 0;

	UPROPERTY()
	TMap<uint64, AActor*> Monsters;

	UPROPERTY(EditAnywhere)
	TSubclassOf<AActor> MonsterClass;


	void SendPacket(SendBufferRef SendBuffer);

public:
	void HandleSpawn(const Protocol::PlayerInfo& PlayerInfo, bool IsMine);
	void HandleSpawn(const Protocol::S_ENTER_GAME& EnterGamePkt);
	void HandleSpawn(const Protocol::S_SPAWN& SpawnPkt);

	void HandleDespawn(uint64 ObjectId);
	void HandleDespawn(const Protocol::S_DESPAWN& DespawnPkt);

	void HandleMove(const Protocol::S_MOVE& MovePkt);

public:
	void HandleSpawnMob(const Protocol::S_SPAWN_MOB& SpawnPkt);
	void HandleDespawnMob(uint64 MobId);
	void HandleDespawnMob(const Protocol::S_DESPAWN_MOB& Pkt);
	void HandleMoveMob(const Protocol::S_MOVE_MOB& MovePkt);
	void HandleDamageMob(const Protocol::S_DAMAGE_MOB& DamagePkt);

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Settings")
	int LandscapeMaterialQualityLevel = 0;

	UFUNCTION(BlueprintCallable, Category = "Game Settings")
	int GetLandscapeMaterialQualityLevel() const { return LandscapeMaterialQualityLevel; }

	UFUNCTION(BlueprintCallable, Category = "Game Settings")
	void SetLandscapeMaterialQualityLevel(int NewQualityLevel) { LandscapeMaterialQualityLevel = NewQualityLevel; }

public:
	class FSocket* Socket;
	FString IpAddress = TEXT("127.0.0.1");
	int16 Port = 7777;
	TSharedPtr<class PacketSession> GameServerSession;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<APlayerCharacter> OtherPlayerClass;

	APlayerCharacter* MyPlayer;
	TMap<uint64, APlayerCharacter*> Players;

private:
	FTimerHandle RecvPacketsTimerHandle;
};
