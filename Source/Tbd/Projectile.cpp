#include "Projectile.h"
#include "MonsterBase.h"
#include "Components/SphereComponent.h"
#include "Components/SceneComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "PlayerCharacter.h"

AProjectile::AProjectile()
{
    PrimaryActorTick.bCanEverTick = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;

    SphereCollision1 = CreateDefaultSubobject<USphereComponent>(TEXT("SphereCollision1"));
    SphereCollision1->SetupAttachment(SceneRoot);

    SphereCollision1->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    SphereCollision1->SetGenerateOverlapEvents(true);
    SphereCollision1->SetCollisionObjectType(ECC_WorldDynamic);
    SphereCollision1->SetCollisionResponseToAllChannels(ECR_Ignore);
    SphereCollision1->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    SphereCollision1->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
    SphereCollision1->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Ignore);

    SphereCollision2 = CreateDefaultSubobject<USphereComponent>(TEXT("SphereCollision2"));
    SphereCollision2->SetupAttachment(SceneRoot);

    SphereCollision2->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    SphereCollision2->SetGenerateOverlapEvents(true);
    SphereCollision2->SetCollisionObjectType(ECC_WorldDynamic);
    SphereCollision2->SetCollisionResponseToAllChannels(ECR_Ignore);
    SphereCollision2->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    SphereCollision2->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);

    BoxCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxCollision"));
    BoxCollision->SetupAttachment(SceneRoot);

    BoxCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    BoxCollision->SetGenerateOverlapEvents(true);
    BoxCollision->SetCollisionObjectType(ECC_WorldDynamic);
    BoxCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
    BoxCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    BoxCollision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);

    ProjectileMove = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMove"));
    ProjectileMove->InitialSpeed = 2000.f;
    ProjectileMove->MaxSpeed = 2000.f;
    ProjectileMove->ProjectileGravityScale = 0.f;
    ProjectileMove->bAutoActivate = false;
    ProjectileMove->SetUpdatedComponent(SphereCollision1);

    SphereCollision1->OnComponentBeginOverlap.AddDynamic(this, &AProjectile::OnProjectileOverlap);
    SphereCollision2->OnComponentBeginOverlap.AddDynamic(this, &AProjectile::OnProjectileOverlap);
    BoxCollision->OnComponentBeginOverlap.AddDynamic(this, &AProjectile::OnProjectileOverlap);
}

void AProjectile::SetHomingTarget(AActor* Target)
{
    HomingTarget = Target;

    bUseHoming = (SkillId == 0 && Target != nullptr);

    UE_LOG(LogTemp, Warning, TEXT("[SetHomingTarget] Target=%s UseHoming=%d"),
        *GetNameSafe(HomingTarget),
        bUseHoming);
}

void AProjectile::BeginPlay()
{
    Super::BeginPlay();

    BaseSphere1Radius = SphereCollision1 ? SphereCollision1->GetUnscaledSphereRadius() : 0.f;
    BaseSphere2Radius = SphereCollision2 ? SphereCollision2->GetUnscaledSphereRadius() : 0.f;
}

void AProjectile::SetProjectileInfo(APlayerCharacter* InOwnerPlayer, int32 InSkillId)
{
    OwnerPlayer = InOwnerPlayer;
    SkillId = InSkillId;
    bLaunched = false;

    SetOwner(InOwnerPlayer);
    ProjectileMove->SetUpdatedComponent(RootComponent);

    switch (SkillId)
    {
    case 0: // Ice
        ProjectileMove->InitialSpeed = 1200.f;
        ProjectileMove->MaxSpeed = 1200.f;
        break;

    case 1: // Fireball
        ProjectileMove->InitialSpeed = 800.f;
        ProjectileMove->MaxSpeed = 800.f;
        break;
    }

    SphereCollision1->IgnoreActorWhenMoving(InOwnerPlayer, true);
    SphereCollision2->IgnoreActorWhenMoving(InOwnerPlayer, true);
    BoxCollision->IgnoreActorWhenMoving(InOwnerPlayer, true);

    SphereCollision1->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SphereCollision2->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BoxCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    UE_LOG(LogTemp, Warning, TEXT("[SetProjectileInfo] Skill=%d Owner=%s"),
        SkillId,
        *GetNameSafe(OwnerPlayer));
}

void AProjectile::ActivateProjectileCollision()
{
    bLaunched = true;

    SphereCollision1->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SphereCollision2->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BoxCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    switch (SkillId)
    {
    case 0: // ICE
        SphereCollision1->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        break;

    case 1: // FIRE
        SphereCollision2->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        break;

    case 2: // KNIGHT
        BoxCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        break;

    default:
        SphereCollision1->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        break;
    }

    UE_LOG(LogTemp, Warning, TEXT("[ActivateProjectileCollision] Skill=%d"), SkillId);
}

void AProjectile::LaunchProjectile(FVector Direction)
{
    if (ProjectileMove)
    {
        FVector LaunchDir = Direction.GetSafeNormal();

        float Speed = ProjectileMove->InitialSpeed;
        if (Speed <= 0.f)
        {
            Speed = 1200.f;
        }

        ProjectileMove->Velocity = LaunchDir * Speed;
        ProjectileMove->Activate();

        SetActorRotation(LaunchDir.Rotation());
    }
}

void AProjectile::SetChargeScale(float InScale)
{
    InScale = FMath::Clamp(InScale, 0.2f, 2.0f);

    SetActorScale3D(FVector(InScale));

    if (SphereCollision1 && BaseSphere1Radius > 0.f)
    {
        SphereCollision1->SetSphereRadius(BaseSphere1Radius * InScale);
    }

    if (SphereCollision2 && BaseSphere2Radius > 0.f)
    {
        SphereCollision2->SetSphereRadius(BaseSphere2Radius * InScale);
    }
}

void AProjectile::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!bUseHoming)
        return;

    if (!bLaunched)
        return;

    if (bHasHit)
        return;

    if (HomingTarget == nullptr)
        return;

    if (ProjectileMove == nullptr)
        return;

    FVector CurrentLocation = GetActorLocation();
    FVector TargetLocation = HomingTarget->GetActorLocation();

    FVector TargetDir = (TargetLocation - CurrentLocation).GetSafeNormal();
    if (TargetDir.IsNearlyZero())
        return;

    FVector CurrentDir = ProjectileMove->Velocity.GetSafeNormal();
    if (CurrentDir.IsNearlyZero())
    {
        CurrentDir = GetActorForwardVector();
    }

    FVector NewDir = FMath::VInterpTo(
        CurrentDir,
        TargetDir,
        DeltaTime,
        HomingInterpSpeed
    ).GetSafeNormal();

    float Speed = ProjectileMove->Velocity.Size();
    if (Speed <= 0.f)
    {
        Speed = ProjectileMove->InitialSpeed;
    }

    ProjectileMove->Velocity = NewDir * Speed;
    SetActorRotation(NewDir.Rotation());
}

void AProjectile::OnProjectileOverlap(
    UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult
)
{
    if (!bLaunched)
        return;

    if (bHasHit)
        return;

    if (OtherActor == nullptr || OtherActor == this)
        return;

    if (OwnerPlayer == nullptr)
        return;

    if (OtherActor == OwnerPlayer || OtherActor == GetOwner() || OtherActor == GetInstigator())
        return;

    if (OwnerPlayer->IsMyPlayer() == false)
        return;

    // 플레이어 피격 판정
    APlayerCharacter* HitPlayer = Cast<APlayerCharacter>(OtherActor);
    if (HitPlayer)
    {
        bHasHit = true;

        UE_LOG(LogTemp, Warning, TEXT("[Projectile] Hit Player Target=%s Owner=%s SkillId=%d"),
            *GetNameSafe(HitPlayer),
            *GetNameSafe(OwnerPlayer),
            SkillId);

        HitPlayer->OnHitBySkill(OwnerPlayer, SkillId);

        PostProcessHit();

        Destroy();
        return;
    }

    // 몬스터 피격 판정
    AMonsterBase* HitMonster = Cast<AMonsterBase>(OtherActor);
    if (HitMonster)
    {
        bHasHit = true;

        UE_LOG(LogTemp, Warning, TEXT("[Projectile] Hit Monster Target=%s Owner=%s SkillId=%d"),
            *GetNameSafe(HitMonster),
            *GetNameSafe(OwnerPlayer),
            SkillId);

        HitMonster->OnHitBySkill(OwnerPlayer, SkillId);

        PostProcessHit();

        Destroy();
        return;
    }
}