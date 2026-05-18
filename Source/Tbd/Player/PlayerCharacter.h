#pragma once

#include <unordered_map>
#include "CoreMinimal.h"
#include "AttributeComponent.h"
#include "UpgradeComponent.h"
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
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	Protocol::PlayerInfo PlayerInfo;
	Protocol::PlayerInfo DestInfo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network")
	bool bIsMine = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	bool bIsDead = false;

	UFUNCTION(BlueprintImplementableEvent)
	void PlaySkill(int32 SkillId);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void BP_PlayIceSkillByHand(bool bUseRightHand);

	UFUNCTION(BlueprintImplementableEvent)
	void PlayOtherPlayerSkill(int32 SkillId);

	UFUNCTION(BlueprintImplementableEvent)
	void PlayOtherPlayerHoldSkill(int32 SkillId);

	UFUNCTION(BlueprintImplementableEvent)
	void PlayDeathAnimation();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Animation")
	void BP_PlayRespawnAnimation();

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
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute")
	UAttributeComponent* AttributeComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
	UUpgradeComponent* UpgradeComponent;

	UPROPERTY(BlueprintReadWrite, Category = "Target")
	AActor* LockedTargetActor = nullptr;

	UPROPERTY(BlueprintReadWrite)
	bool bIsHomingSkillMine = false;

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void Req_UseIceSkill(int32 SkillId, AActor* Target);



	// Network Attack Animation
	UFUNCTION(BlueprintCallable, Category = "Network")
	void PlayNetworkAttackAnimation();

private:
	void ApplyNetworkPosition(float DeltaTime);
};
