#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Character.h"
#include "MonsterBase.generated.h"

class APlayerCharacter;

UCLASS()
class TBD_API AMonsterBase : public ACharacter
{
	GENERATED_BODY()

public:
	AMonsterBase();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster")
	int32 MonsterId = 0;

protected:
	virtual void BeginPlay() override;

public:
	UFUNCTION(BlueprintCallable)
	void OnHitBySkill(APlayerCharacter* Attacker, int32 SkillId);

	UFUNCTION(BlueprintCallable)
	void NotifyMonsterDead(APlayerCharacter* Killer);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void RecvDamageFromSkill(float Damage, APlayerCharacter* Attacker, int32 SkillId, FGameplayTag DamageType);
};