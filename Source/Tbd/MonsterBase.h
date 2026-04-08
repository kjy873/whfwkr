#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MonsterBase.generated.h"

class APlayerCharacter;

UCLASS()
class TBD_API AMonsterBase : public ACharacter
{
	GENERATED_BODY()

public:
	AMonsterBase();

public:
	UFUNCTION(BlueprintImplementableEvent)
	void BP_OnHitBySkill(APlayerCharacter* Attacker, int32 SkillId);
};