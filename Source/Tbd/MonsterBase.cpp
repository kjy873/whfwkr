#include "MonsterBase.h"
#include "PlayerCharacter.h"
#include "MainGameInstance.h"

AMonsterBase::AMonsterBase()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AMonsterBase::BeginPlay()
{
	Super::BeginPlay();
}

void AMonsterBase::OnHitBySkill(APlayerCharacter* Attacker, int32 SkillId)
{
    float Damage = 10.0f;
    FGameplayTag DamageType;

    switch (SkillId)
    {
    case 0:
        Damage = 20.0f;
        DamageType = FGameplayTag::RequestGameplayTag(FName("DamageType.Ice"));
        break;

    case 1:
        Damage = 35.0f;
        DamageType = FGameplayTag::RequestGameplayTag(FName("DamageType.Fire"));
        break;

    default:
        Damage = 5.0f;
        break;
    }

    UE_LOG(LogTemp, Warning, TEXT("[MonsterBase] RecvDamageFromSkill Target=%s Attacker=%s SkillId=%d Damage=%f DamageType=%s"),
        *GetNameSafe(this),
        *GetNameSafe(Attacker),
        SkillId,
        Damage,
        *DamageType.ToString());

    RecvDamageFromSkill(Damage, Attacker, SkillId, DamageType);
}

void AMonsterBase::NotifyMonsterDead(APlayerCharacter* Killer)
{
    if (Killer == nullptr)
        return;

    if (!Killer->IsMyPlayer())
        return;

    UMainGameInstance* GI = Cast<UMainGameInstance>(GetGameInstance());
    if (GI == nullptr)
        return;

    GI->SendMonsterKill();

    UE_LOG(LogTemp, Warning, TEXT("[NotifyMonsterDead] Killer=%s Monster=%s"),
        *GetNameSafe(Killer),
        *GetNameSafe(this));
}