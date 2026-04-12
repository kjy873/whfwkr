#include "Projectile.h"
#include "PlayerCharacter.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	// Sphere 1
	SphereCollision1 = CreateDefaultSubobject<USphereComponent>(TEXT("SphereCollision1"));
	SphereCollision1->SetupAttachment(RootComponent);
	SphereCollision1->InitSphereRadius(20.f);
	SphereCollision1->SetCollisionObjectType(ECC_WorldDynamic);
	SphereCollision1->SetCollisionResponseToAllChannels(ECR_Ignore);
	SphereCollision1->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SphereCollision1->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Overlap);
	SphereCollision1->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	SphereCollision1->SetGenerateOverlapEvents(true);

	// Sphere 2
	SphereCollision2 = CreateDefaultSubobject<USphereComponent>(TEXT("SphereCollision2"));
	SphereCollision2->SetupAttachment(RootComponent);
	SphereCollision2->InitSphereRadius(40.f);
	SphereCollision2->SetRelativeLocation(FVector(50.f, 0.f, 0.f));
	SphereCollision2->SetCollisionObjectType(ECC_WorldDynamic);
	SphereCollision2->SetCollisionResponseToAllChannels(ECR_Ignore);
	SphereCollision2->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SphereCollision2->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Overlap);
	SphereCollision2->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	SphereCollision2->SetGenerateOverlapEvents(true);

	// Box
	BoxCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxCollision"));
	BoxCollision->SetupAttachment(RootComponent);
	BoxCollision->SetBoxExtent(FVector(40.f, 20.f, 20.f));
	BoxCollision->SetRelativeLocation(FVector(60.f, 0.f, 0.f));
	BoxCollision->SetCollisionObjectType(ECC_WorldDynamic);
	BoxCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	BoxCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	BoxCollision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Overlap);
	BoxCollision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	BoxCollision->SetGenerateOverlapEvents(true);

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

	if (SphereCollision1)
	{
		SphereCollision1->OnComponentBeginOverlap.AddDynamic(this, &AProjectile::OnProjectileOverlap);
	}

	if (SphereCollision2)
	{
		SphereCollision2->OnComponentBeginOverlap.AddDynamic(this, &AProjectile::OnProjectileOverlap);
	}

	if (BoxCollision)
	{
		BoxCollision->OnComponentBeginOverlap.AddDynamic(this, &AProjectile::OnProjectileOverlap);
	}
}

void AProjectile::SetProjectileInfo(APlayerCharacter* InOwnerPlayer, int32 InSkillId)
{
	OwnerPlayer = InOwnerPlayer;
	SkillId = InSkillId;

	switch (SkillId)
	{
	case 0: // ICE
		ProjectileMove->SetUpdatedComponent(SphereCollision1);
		break;

	case 1: // FIRE
		ProjectileMove->SetUpdatedComponent(SphereCollision1);
		break;

	case 2: // KNIGHT
		ProjectileMove->SetUpdatedComponent(BoxCollision);
		break;

	default:
		ProjectileMove->SetUpdatedComponent(SphereCollision1);
		break;
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
	if (bHasHit)
		return;

	if (OtherActor == nullptr || OtherActor == this)
		return;

	if (OtherActor == OwnerPlayer)
		return;

	bHasHit = true;

	UE_LOG(LogTemp, Warning, TEXT("[Projectile] Hit: %s / Collision: %s"),
		*GetNameSafe(OtherActor),
		*GetNameSafe(OverlappedComponent));

	APlayerCharacter* HitPlayer = Cast<APlayerCharacter>(OtherActor);
	if (HitPlayer && OwnerPlayer)
	{
		HitPlayer->OnHitBySkill(OwnerPlayer, SkillId);
	}

	DebugFunc();

	Destroy();
}