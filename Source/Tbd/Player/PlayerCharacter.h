#pragma once

#include <unordered_map>
#include "CoreMinimal.h"
#include "AttributeComponent.h"
#include "GameFramework/Character.h"
#include "Protocol.pb.h"
#include "Attributes.h"
#include "PlayerCharacter.generated.h"

using namespace std;

UCLASS()
class TBD_API APlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	APlayerCharacter();
	virtual ~APlayerCharacter() override;

	virtual void Tick(float DeltaTime) override;
	virtual void BeginPlay() override;

	Protocol::PlayerInfo PlayerInfo;
	Protocol::PlayerInfo DestInfo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network")
	bool bIsMine = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	bool bIsDead = false;

	UFUNCTION(BlueprintImplementableEvent)
	void PlaySkill(int32 SkillId);

	UFUNCTION(BlueprintImplementableEvent)
	void PlayOtherPlayerSkill(int32 SkillId);

	UFUNCTION(BlueprintImplementableEvent)
	void PlayDeathAnimation();

	UFUNCTION(BlueprintImplementableEvent)
	void PlayHitReaction();

	UFUNCTION()
	void OnHitBySkill(APlayerCharacter* Attacker, int32 SkillId);

	void SetPlayerInfo(const Protocol::PlayerInfo& Info);
	void SetDestInfo(const Protocol::PlayerInfo& Info);

	UFUNCTION(BlueprintCallable, Category = "State")
	void SetDead(bool bDead);

	UFUNCTION(BlueprintCallable)
	bool IsMyPlayer() const { return bIsMine; }
	
	// Attrubutes begin
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target")
	UAttributeComponent* AttributeComponent;

	UPROPERTY(BlueprintReadWrite, Category = "Target")
	AActor* LockedTargetActor = nullptr;

	UPROPERTY(BlueprintReadWrite)
	bool bIsHomingSkillMine = false;

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void Req_UseIceSkill(int32 SkillId, AActor* Target);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	FPlayerUpgradeState UpgradeState;

	UFUNCTION(BlueprintCallable, Category = "UpgradeFunctions")
	void AddUpgrade(const FName& UpgradeName) { UpgradeState.AddUpgrade(UpgradeName); }

	UFUNCTION(BlueprintCallable, Category = "UpgradeFunctions")
	bool HasUpgrade(const FName& UpgradeName) const { return UpgradeState.HasUpgrade(UpgradeName); }

	UFUNCTION(BlueprintCallable, Category = "UpgradeFunctions")
	int GetUpgradeCount(const FName& UpgradeName) const { return UpgradeState.GetUpgradeCount(UpgradeName); }

	UFUNCTION(BlueprintCallable, Category = "UpgradeFunctions")
	void SetUpgrades(const TMap<FName, int>& Src) { UpgradeState.SetUpgrades(Src); }

	UFUNCTION(BlueprintCallable, Category = "UpgradeFunctions")
	TMap<FName, int> GetUpgrades() const { return UpgradeState.AquiredUpgrades; }

	// Attributes end



	// Network Attack Animation
	UFUNCTION(BlueprintCallable, Category = "Network")
	void PlayNetworkAttackAnimation();

private:
	void ApplyNetworkPosition(float DeltaTime);
};
