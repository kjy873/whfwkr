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

    SphereCollision1->SetCollisionObjectType(ECC_WorldDynamic);
    SphereCollision1->SetCollisionResponseToAllChannels(ECR_Ignore);
    SphereCollision1->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    SphereCollision1->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
    SphereCollision1->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Ignore);

    SphereCollision1->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    SphereCollision1->SetGenerateOverlapEvents(true);


    SphereCollision2 = CreateDefaultSubobject<USphereComponent>(TEXT("SphereCollision2"));
    SphereCollision2->SetupAttachment(SceneRoot);

    SphereCollision2->SetCollisionObjectType(ECC_WorldDynamic);
    SphereCollision2->SetCollisionResponseToAllChannels(ECR_Ignore);
    SphereCollision2->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    SphereCollision2->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
    SphereCollision2->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Ignore);

    SphereCollision2->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    SphereCollision2->SetGenerateOverlapEvents(true);


    BoxCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxCollision"));
    BoxCollision->SetupAttachment(SceneRoot);

    BoxCollision->SetCollisionObjectType(ECC_WorldDynamic);
    BoxCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
    BoxCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    BoxCollision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
    BoxCollision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Ignore);

    BoxCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    BoxCollision->SetGenerateOverlapEvents(true);


    ProjectileMove = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMove"));
    ProjectileMove->InitialSpeed = 2000.f;
    ProjectileMove->MaxSpeed = 2000.f;
    ProjectileMove->ProjectileGravityScale = 0.f;
    ProjectileMove->bAutoActivate = false;

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

void AProjectile::SetProjectileInfo(int32 InSkillId, AActor* InOwnerPlayer)
{
    SkillId = InSkillId;
    OwnerPlayer = InOwnerPlayer;
    SetOwner(InOwnerPlayer);

    if (OwnerPlayer)
    {
        if (SphereCollision1)
            SphereCollision1->IgnoreActorWhenMoving(OwnerPlayer, true);

        if (SphereCollision2)
            SphereCollision2->IgnoreActorWhenMoving(OwnerPlayer, true);

        if (BoxCollision)
            BoxCollision->IgnoreActorWhenMoving(OwnerPlayer, true);
    }

    UE_LOG(LogTemp, Warning, TEXT("[SetProjectileInfo] Skill=%d Owner=%s GetOwner=%s"),
        SkillId,
        *GetNameSafe(OwnerPlayer),
        *GetNameSafe(GetOwner()));
}

void AProjectile::ActivateProjectileCollision()
{
    bHasHit = false;
    bLaunched = true;
     
    MoveDirection = GetActorForwardVector().GetSafeNormal();

    if (MoveDirection.IsNearlyZero())
    {
        MoveDirection = FVector::ForwardVector;
    }

    UE_LOG(LogTemp, Warning, TEXT("[ActivateProjectileCollision] Skill=%d Owner=%s Dir=%s"),
        SkillId,
        *GetNameSafe(OwnerPlayer),
        *MoveDirection.ToString());
}

void AProjectile::LaunchProjectile(const FVector& Direction)
{
    bHasHit = false;
    bLaunched = true;

    MoveDirection = Direction.GetSafeNormal();

    if (MoveDirection.IsNearlyZero())
    {
        MoveDirection = GetActorForwardVector().GetSafeNormal();
    }

    if (MoveDirection.IsNearlyZero())
    {
        MoveDirection = FVector::ForwardVector;
    }

    SetActorRotation(MoveDirection.Rotation());

    UE_LOG(LogTemp, Warning, TEXT("[LaunchProjectile] Skill=%d Owner=%s Dir=%s Speed=%f Loc=%s"),
        SkillId,
        *GetNameSafe(OwnerPlayer),
        *MoveDirection.ToString(),
        MoveSpeed,
        *GetActorLocation().ToString());
}

void AProjectile::SetChargeScale(float InScale)
{
    InScale = FMath::Clamp(InScale, 1.0f, 3.0f);

    UE_LOG(LogTemp, Warning, TEXT("[Projectile SetChargeScale] InScale=%f"), InScale);

    SetActorScale3D(FVector(InScale));

    /*
    if (SphereCollision1 && BaseSphere1Radius > 0.f)
    {
        SphereCollision1->SetSphereRadius(BaseSphere1Radius * InScale);
    }

    if (SphereCollision2 && BaseSphere2Radius > 0.f)
    {
        SphereCollision2->SetSphereRadius(BaseSphere2Radius * InScale);
    }
    */
}

void AProjectile::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!bLaunched)
        return;

    if (bHasHit)
        return;

    if (bUseHoming && HomingTarget != nullptr)
    {
        FVector CurrentLocation = GetActorLocation();
        FVector TargetLocation = HomingTarget->GetActorLocation();

        FVector TargetDir = (TargetLocation - CurrentLocation).GetSafeNormal();

        if (!TargetDir.IsNearlyZero())
        {
            FVector CurrentDir = MoveDirection.GetSafeNormal();

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

            if (!NewDir.IsNearlyZero())
            {
                MoveDirection = NewDir;
                SetActorRotation(MoveDirection.Rotation());
            }
        }
    }

    FVector FinalDirection = MoveDirection.GetSafeNormal();

    if (FinalDirection.IsNearlyZero())
    {
        FinalDirection = GetActorForwardVector().GetSafeNormal();
        MoveDirection = FinalDirection;
    }

    AddActorWorldOffset(FinalDirection * MoveSpeed * DeltaTime, false);

    UE_LOG(LogTemp, Warning, TEXT("[Projectile Tick] Loc=%s Dir=%s Speed=%f"),
        *GetActorLocation().ToString(),
        *FinalDirection.ToString(),
        MoveSpeed);

    CheckManualOverlap();
}

void AProjectile::CheckManualOverlap()
{
    if (!bLaunched || bHasHit)
        return;

    UWorld* World = GetWorld();
    if (World == nullptr)
        return;

    FVector CheckLocation = GetActorLocation();

    if (SphereCollision1)
    {
        CheckLocation = SphereCollision1->GetComponentLocation();
    }

    float Radius = HitCheckRadius;

    if (SphereCollision1)
    {
        Radius = FMath::Max(HitCheckRadius, SphereCollision1->GetScaledSphereRadius());
    }

    TArray<FOverlapResult> OverlapResults;

    FCollisionObjectQueryParams ObjectParams;
    ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
    ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);

    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);

    if (OwnerPlayer)
    {
        QueryParams.AddIgnoredActor(OwnerPlayer);
    }

    bool bOverlapped = World->OverlapMultiByObjectType(
        OverlapResults,
        CheckLocation,
        FQuat::Identity,
        ObjectParams,
        FCollisionShape::MakeSphere(Radius),
        QueryParams
    );

    if (!bOverlapped)
        return;

    for (const FOverlapResult& Result : OverlapResults)
    {
        AActor* OtherActor = Result.GetActor();

        if (OtherActor == nullptr)
            continue;

        if (OtherActor == this)
            continue;

        if (OtherActor == OwnerPlayer || OtherActor == GetOwner())
            continue;

        UE_LOG(LogTemp, Warning, TEXT("[Projectile ManualOverlap HIT] Self=%s Other=%s Owner=%s SkillId=%d Radius=%f"),
            *GetNameSafe(this),
            *GetNameSafe(OtherActor),
            *GetNameSafe(OwnerPlayer),
            SkillId,
            Radius);

        HandleProjectileHit(OtherActor);
        return;
    }
}

void AProjectile::HandleProjectileHit(AActor* OtherActor)
{
    if (!bLaunched)
        return;

    if (bHasHit)
        return;

    if (OtherActor == nullptr || OtherActor == this)
        return;

    if (OtherActor == OwnerPlayer || OtherActor == GetOwner())
        return;

    APlayerCharacter* OwnerCharacter = Cast<APlayerCharacter>(OwnerPlayer);

    if (OwnerCharacter == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HandleProjectileHit RETURN] OwnerCharacter cast failed Owner=%s"),
            *GetNameSafe(OwnerPlayer));
        return;
    }

    APlayerCharacter* HitPlayer = Cast<APlayerCharacter>(OtherActor);
    if (HitPlayer)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HandleProjectileHit] Hit Player Target=%s Owner=%s SkillId=%d"),
            *GetNameSafe(HitPlayer),
            *GetNameSafe(OwnerCharacter),
            SkillId);

        bHasHit = true;
        HitPlayer->OnHitBySkill(OwnerCharacter, SkillId);
        PostProcessHit();
        Destroy();
        return;
    }

    AMonsterBase* HitMonster = Cast<AMonsterBase>(OtherActor);
    if (HitMonster)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HandleProjectileHit] Hit Monster Target=%s Owner=%s SkillId=%d"),
            *GetNameSafe(HitMonster),
            *GetNameSafe(OwnerCharacter),
            SkillId);

        bHasHit = true;
        HitMonster->OnHitBySkill(OwnerCharacter, SkillId);
        PostProcessHit();
        Destroy();
        return;
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
    UE_LOG(LogTemp, Warning, TEXT("[Projectile Overlap ENTER] Self=%s Other=%s OwnerPlayer=%s GetOwner=%s bLaunched=%d bHasHit=%d SkillId=%d"),
        *GetNameSafe(this),
        *GetNameSafe(OtherActor),
        *GetNameSafe(OwnerPlayer),
        *GetNameSafe(GetOwner()),
        bLaunched,
        bHasHit,
        SkillId);

    HandleProjectileHit(OtherActor);

    if (!bLaunched)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Projectile Overlap RETURN] Not launched"));
        return;
    }

    if (bHasHit)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Projectile Overlap RETURN] Already hit"));
        return;
    }

    if (OtherActor == nullptr || OtherActor == this)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Projectile Overlap RETURN] Null or self"));
        return;
    }

    if (OtherActor == OwnerPlayer || OtherActor == GetOwner())
    {
        UE_LOG(LogTemp, Warning, TEXT("[Projectile Overlap RETURN] Owner ignored"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("[Projectile Damage TRY] Other=%s SkillId=%d"),
        *GetNameSafe(OtherActor),
        SkillId);

    bHasHit = true;

    APlayerCharacter* OwnerCharacter = Cast<APlayerCharacter>(OwnerPlayer);

    if (OwnerCharacter == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Projectile Overlap RETURN] OwnerCharacter cast failed OwnerPlayer=%s"),
            *GetNameSafe(OwnerPlayer));
        return;
    }

    // 플레이어 피격 판정
    APlayerCharacter* HitPlayer = Cast<APlayerCharacter>(OtherActor);
    if (HitPlayer)
    {
        bHasHit = true;

        UE_LOG(LogTemp, Warning, TEXT("[Projectile] Hit Player Target=%s Owner=%s SkillId=%d"),
            *GetNameSafe(HitPlayer),
            *GetNameSafe(OwnerPlayer),
            SkillId);

        HitPlayer->OnHitBySkill(OwnerCharacter, SkillId);    

        UE_LOG(LogTemp, Warning, TEXT("[Projectile Destroy] Hit Player=%s"),
            *GetNameSafe(HitPlayer));

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

        HitMonster->OnHitBySkill(OwnerCharacter, SkillId);

        UE_LOG(LogTemp, Warning, TEXT("[Projectile Destroy] Hit Monster=%s"),
            *GetNameSafe(HitMonster));

        Destroy();
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("[Projectile Overlap NO HIT] Other=%s Class=%s"),
        *GetNameSafe(OtherActor),
        *GetNameSafe(OtherActor->GetClass()));
}