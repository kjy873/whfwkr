#pragma once

#include <unordered_map>
#include "CoreMinimal.h"
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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
	FAttribute Attributes;

	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	float GetHealthPercent() { return Attributes.GetHealthPercent(); }
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	float GetStaminaPercent() { return Attributes.GetStaminaPercent(); }
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	float GetManaPercent() { return Attributes.GetManaPercent(); }
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	float GetExperiencePercent() { return Attributes.GetExperiencePercent(); }

	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	void AddHealth(float Value) { Attributes.AddHealth(Value); }
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	void AddStamina(float Value) { Attributes.AddStamina(Value); }
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	void AddMana(float Value) { Attributes.AddMana(Value); }
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	bool AddExperience(float Value) { return Attributes.AddExperience(Value); }
	
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	void AddMaxHealth(float Value) { Attributes.AddMaxHealth(Value); }
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	void AddMaxStamina(float Value) { Attributes.AddMaxStamina(Value); }
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	void AddMaxMana(float Value) { Attributes.AddMaxMana(Value); }

	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	void SubtractHealth(float Value) { Attributes.SubtractHealth(Value); }
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	void SubtractStamina(float Value) { Attributes.SubtractStamina(Value); }
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	void SubtractMana(float Value) { Attributes.SubtractMana(Value); }
	
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	void AddBaseDamage(float Value) { Attributes.AddBaseDamage(Value); }

	UPROPERTY(BlueprintReadWrite, Category = "Target")
	AActor* LockedTargetActor = nullptr;

	UPROPERTY(BlueprintReadWrite)
	bool bIsHomingSkillMine = false;

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void Req_UseIceSkill(int32 SkillId, AActor* Target);

	// Attributes end

	// Network Attack Animation
	UFUNCTION(BlueprintCallable, Category = "Network")
	void PlayNetworkAttackAnimation();

private:
	void ApplyNetworkPosition(float DeltaTime);
};
