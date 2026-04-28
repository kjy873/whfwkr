#include "Projectile.h"
#include "Components/SphereComponent.h"
#include "Components/SceneComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "PlayerCharacter.h"

AProjectile::AProjectile()
{
    PrimaryActorTick.bCanEverTick = false;

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
        ProjectileMove->Velocity = Direction * 2000.f;
        ProjectileMove->Activate();
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

    if (OtherActor == OwnerPlayer || OtherActor == GetOwner() || OtherActor == GetInstigator())
        return;

    bHasHit = true;

    APlayerCharacter* HitPlayer = Cast<APlayerCharacter>(OtherActor);

    UE_LOG(LogTemp, Warning, TEXT("[Projectile] Before OnHitBySkill Target=%s Owner=%s SkillId=%d"),
        *GetNameSafe(HitPlayer),
        *GetNameSafe(OwnerPlayer),
        SkillId);

    if (HitPlayer && OwnerPlayer)
    {
        HitPlayer->OnHitBySkill(OwnerPlayer, SkillId);
    }

    Destroy();
}