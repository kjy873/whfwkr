// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Tbd.h"
#include "HAL/PlatformProcess.h"
#include "Protocol.pb.h"
#include "Player/PlayerCharacter.h"
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

	UFUNCTION(BlueprintCallable)
	void SendEnterGamePacket();

	UFUNCTION(BlueprintCallable)
	void ClearPlayerStateForLevelChange();

	UPROPERTY(BlueprintReadWrite)
	bool bChangingLevel = false;

	UFUNCTION(BlueprintCallable)
	void NotifyLevelLoadFinished();

	UFUNCTION(BlueprintCallable)
	void StartGameConnection();

	void HandleDamage(const Protocol::S_DAMAGE_PLAYER& pkt);
	void HandleDie(const Protocol::S_PLAYER_DEAD& Pkt);
	void SendAttackPlayer(uint64 TargetId, uint32 SkillId);

	void OnRecvUseSkill(const Protocol::S_USE_SKILL& pkt);
	void HandleIceSkillPacket(uint64 CasterID, uint64 TargetID);

	virtual void Init() override;
	virtual void Shutdown() override;

	UPROPERTY()
	uint64 MyObjectId = 0;

	UPROPERTY()
	TMap<uint64, AActor*> Monsters;

	UPROPERTY(EditAnywhere)
	TSubclassOf<AActor> MonsterClass;

	APlayerCharacter* GetPlayerById(uint64 PlayerId);

	void SendPacket(SendBufferRef SendBuffer);
	void SendLevelReady();

public:
	void HandleSpawn(const Protocol::PlayerInfo& PlayerInfo, bool IsMine);
	void HandleSpawn(const Protocol::S_ENTER_GAME& EnterGamePkt);
	void HandleSpawn(const Protocol::S_SPAWN& SpawnPkt);

	void HandleDespawn(uint64 ObjectId);
	void HandleDespawn(const Protocol::S_DESPAWN& DespawnPkt);

	void HandleMove(const Protocol::S_MOVE& MovePkt);

public:
	void HandleDespawnMob(uint64 MobId);
	void HandleDespawnMob(const Protocol::S_DESPAWN_MOB& Pkt);
	void HandleMoveMob(const Protocol::S_MOVE_MOB& MovePkt);
	void HandleDamageMob(const Protocol::S_DAMAGE_MOB& DamagePkt);
	void OnRecvProjectileHit(const Protocol::S_PROJECTILE_HIT& pkt);
	void OnRecvProjectileDestroy(const Protocol::S_PROJECTILE_DESTROY& pkt);

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

	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	TSubclassOf<AActor> IceProjectileBPClass;
	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	TSubclassOf<AActor> FireballProjectileBPClass;

	TWeakObjectPtr<APlayerCharacter> MyPlayer;
	TMap<uint64, TWeakObjectPtr<APlayerCharacter>> Players;

	TMap<int32, AActor*> PredictedByShotId;
	TMap<int32, AActor*> ByProjectileId;
	TArray<Protocol::PlayerInfo> PendingSpawns;

private:
	FTimerHandle RecvPacketsTimerHandle;

	void StartServerProcess();
	void StopServerProcess();

	FProcHandle ServerProcHandle;
	bool bStartedServer = false;

private:

	UPROPERTY(EditAnywhere, Category = "PlayerData")
	ECharacterType LocalPlayerCharacterType;

	UPROPERTY(EditAnywhere, Category = "PlayerData")
	FAttribute LocalPlayerAttributes;

	UPROPERTY(EditAnywhere, Category = "PlayerData")
	TSet<EUnlockType> LocalPlayerUnlockedSet;
public:
	UFUNCTION(BlueprintCallable, Category = "PlayerData")
	ECharacterType GetCharacterType() const { return LocalPlayerCharacterType; }
	UFUNCTION(BlueprintCallable, Category = "PlayerData")
	void SetCharacterType(ECharacterType NewType) { LocalPlayerCharacterType = NewType; }
private:
	UPROPERTY(EditAnywhere, Category = "PlayerData")
	FUpgrades LocalPlayerUpgrades;

public:

	UFUNCTION(BlueprintCallable, Category = "PlayerData")
	const TMap<FName, int>& GetUpgrades() const { return LocalPlayerUpgrades.UpgradeCounts; }
	UFUNCTION(BlueprintCallable, Category = "PlayerData")
	void SetUpgrades(const TMap<FName, int>& NewUpgrades) { LocalPlayerUpgrades.UpgradeCounts = NewUpgrades; }
	UFUNCTION(BlueprintCallable, Category = "PlayerData")
	void InitUpgrades(UDataTable* DataTable, ECharacterType CharacterType) { LocalPlayerUpgrades.InitUpgrades(DataTable, CharacterType); }
	UFUNCTION(BlueprintCallable, Category = "PlayerData")
	void AddUpgrade(const FName& UpgradeName) { LocalPlayerUpgrades.AddUpgrade(UpgradeName); }
	UFUNCTION(BlueprintCallable, Category = "PlayerData")
	bool HasUpgrade(const FName& UpgradeName) const { return LocalPlayerUpgrades.HasUpgrade(UpgradeName); }
	UFUNCTION(BlueprintCallable, Category = "PlayerData")
	TArray<FName> GetAvailableUpgrades() const { return LocalPlayerUpgrades.GetAvailableUpgrades(); }


	UFUNCTION(BlueprintCallable, Category = "PlayerData")
	FAttribute GetLocalPlayerAttributes() const { return LocalPlayerAttributes; }
	UFUNCTION(BlueprintCallable, Category = "PlayerData")
	void SetLocalPlayerAttributes(const FAttribute& NewAttributes) { LocalPlayerAttributes = NewAttributes; }

};