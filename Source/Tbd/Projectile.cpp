#include "Projectile.h"
#include "PlayerCharacter.h"
#include "MonsterBase.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"

AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComp"));
	RootComponent = CollisionComp;

	CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComp->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionComp->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Overlap);
	CollisionComp->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);

	ProjectileMove = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMove"));
	ProjectileMove->InitialSpeed = 1500.f;
	ProjectileMove->MaxSpeed = 1500.f;
	ProjectileMove->ProjectileGravityScale = 0.f;
	ProjectileMove->bRotationFollowsVelocity = true;

	InitialLifeSpan = 5.f;
}

void AProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (CollisionComp)
	{
		CollisionComp->OnComponentBeginOverlap.AddDynamic(this, &AProjectile::OnProjectileOverlap);
	}
}

void AProjectile::SetProjectileInfo(APlayerCharacter* InOwnerPlayer, int32 InSkillId)
{
	OwnerPlayer = InOwnerPlayer;
	SkillId = InSkillId;

	SetOwner(InOwnerPlayer);
	SetInstigator(InOwnerPlayer);

	UE_LOG(LogTemp, Warning, TEXT("[SetProjectileInfo] OwnerPlayer=%s SkillId=%d"),
		*GetNameSafe(OwnerPlayer),
		SkillId);
}

void AProjectile::OnProjectileOverlap(
    UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult)
{
    if (OtherActor == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("OtherActor NULL"));
        return;
    }

    AActor* HitActor = OtherActor;

    if (OtherComp)
    {
        AActor* CompOwner = OtherComp->GetOwner();
        if (CompOwner)
        {
            HitActor = CompOwner;
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("Hit Actor: %s"), *GetNameSafe(HitActor));
    UE_LOG(LogTemp, Warning, TEXT("Hit Class: %s"), *GetNameSafe(HitActor->GetClass()));
    UE_LOG(LogTemp, Warning, TEXT("Projectile Owner: %s"), *GetNameSafe(GetOwner()));
    UE_LOG(LogTemp, Warning, TEXT("Projectile Instigator: %s"), *GetNameSafe(GetInstigator()));

    if (HitActor == this)
    {
        return;
    }

    if (HitActor == GetOwner())
    {
        return;
    }

    APlayerCharacter* Attacker = OwnerPlayer;

    if (Attacker == nullptr)
    {
        Attacker = Cast<APlayerCharacter>(GetOwner());
    }

    if (Attacker == nullptr)
    {
        Attacker = Cast<APlayerCharacter>(GetInstigator());
    }

    if (Attacker == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("OwnerPlayer NULL"));
        Destroy();
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("OwnerPlayer OK: %s"), *GetNameSafe(Attacker));

    APlayerCharacter* Target = Cast<APlayerCharacter>(HitActor);
    if (Target)
    {
        UE_LOG(LogTemp, Warning, TEXT("Target Cast Success"));

        Target->OnHitBySkill(Attacker, SkillId);
        UE_LOG(LogTemp, Warning, TEXT("Player Hit: %s"), *Target->GetName());

        Destroy();
        return;
    }

    AMonsterBase* Monster = Cast<AMonsterBase>(HitActor);
    if (Monster)
    {
        UE_LOG(LogTemp, Warning, TEXT("Monster Hit: %s"), *GetNameSafe(HitActor));

        Monster->BP_OnHitBySkill(Attacker, SkillId);

        UGameplayStatics::ApplyDamage(
            Monster,
            30.0f,
            Attacker->GetController(),
            this,
            UDamageType::StaticClass()
        );

        Destroy();
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("Cast Failed"));
    Destroy();
}