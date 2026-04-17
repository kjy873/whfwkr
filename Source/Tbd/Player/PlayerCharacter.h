#pragma once

#include <unordered_map>
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Protocol.pb.h"
#include "MainPlayerState.h"
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

	virtual void PossessedBy(AController* NewController) override;
	UFUNCTION(BlueprintImplementableEvent)
	void OnInit();

	Protocol::PlayerInfo PlayerInfo;
	Protocol::PlayerInfo DestInfo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network")
	bool bIsMine = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	bool bIsDead = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OnRep")
	bool PawnReady = false;
	bool ControllerReady = false;
	bool PlayerStateReady = false;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute")
	TWeakObjectPtr<AMainPlayerState> MainPlayerState;

	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	float GetHealthPercent() { return MainPlayerState->GetHealthPercent(); }
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	float GetStaminaPercent() { return MainPlayerState->GetStaminaPercent(); }
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	float GetManaPercent() { return MainPlayerState->GetManaPercent(); }
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	float GetExperiencePercent() { return MainPlayerState->GetExperiencePercent(); }

	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	void AddHealth(float Value) { MainPlayerState->AddHealth(Value); }
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	void AddStamina(float Value) { MainPlayerState->AddStamina(Value); }
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	void AddMana(float Value) { MainPlayerState->AddMana(Value); }
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	bool AddExperience(float Value) { return MainPlayerState->AddExperience(Value); }
	
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	void AddMaxHealth(float Value) { MainPlayerState->AddMaxHealth(Value); }
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	void AddMaxStamina(float Value) { MainPlayerState->AddMaxStamina(Value); }
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	void AddMaxMana(float Value) { MainPlayerState->AddMaxMana(Value); }

	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	void SubtractHealth(float Value) { MainPlayerState->SubtractHealth(Value); }
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	void SubtractStamina(float Value) { MainPlayerState->SubtractStamina(Value); }
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	void SubtractMana(float Value) { MainPlayerState->SubtractMana(Value); }
	
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	void AddBaseDamage(float Value) { MainPlayerState->AddBaseDamage(Value); }

	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	float GetBaseDamage() { return MainPlayerState->GetBaseDamage(); }

	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	int GetCharacterLevel() { return MainPlayerState->GetCharacterLevel(); }

	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	int GetStrength() const { return MainPlayerState->GetStrength(); }
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	int GetAgility() const { return MainPlayerState->GetAgility(); }
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	int GetIntelligence() const { return MainPlayerState->GetIntelligence(); }

	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	void AddStrength(int Value) { MainPlayerState->AddStrength(Value); }
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	void AddAgility(int Value) { MainPlayerState->AddAgility(Value); }
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	void AddIntelligence(int Value) { MainPlayerState->AddIntelligence(Value); }

	
	// Attributes end

	// Unlock begin
	UFUNCTION(BlueprintCallable, Category = "Unlock")
	bool HasUnlocked(EUnlockType Type) { return MainPlayerState->HasUnlock(Type); }
	UFUNCTION(BlueprintCallable, Category = "Unlock")
	void Unlock(EUnlockType Type) { MainPlayerState->Unlock(Type); }
	UFUNCTION(BlueprintCallable, Category = "Unlock")
	void Lock(EUnlockType Type) { MainPlayerState->Lock(Type); }
	// Unlock end

	virtual void OnRep_PlayerState() override;

	UFUNCTION(BlueprintCallable, Category = "Initialize")
	void OnPlayerStateReady();


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
