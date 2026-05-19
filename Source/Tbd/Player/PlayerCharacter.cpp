#include "Tbd/Player/PlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "MainGameInstance.h"

// Sets default values
APlayerCharacter::APlayerCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	AttributeComponent = CreateDefaultSubobject<UAttributeComponent>(TEXT("AttributeComponent"));
	UpgradeComponent = CreateDefaultSubobject<UUpgradeComponent>(TEXT("UpgradeComponent"));
}

APlayerCharacter::~APlayerCharacter()
{
}

// Called when the game starts or when spawned
void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("[PlayerCharacter BeginPlay] %s / ObjId=%lld"),
		*GetName(),
		(int64)PlayerInfo.object_id());
}

// Called every frame

void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	ApplyNetworkPosition(DeltaTime);
}

void APlayerCharacter::OnHitBySkill(APlayerCharacter* Attacker, int32 SkillId)
{
	const uint64 TargetObjId = PlayerInfo.object_id();
	const uint64 AttackerObjId = Attacker ? Attacker->PlayerInfo.object_id() : 0;

	UE_LOG(LogTemp, Warning, TEXT("[OnHitBySkill] Called Target=%s TargetObjId=%llu TargetDead=%d Attacker=%s AttackerObjId=%llu AttackerIsMine=%d SkillId=%d"),
		*GetName(),
		(unsigned long long)TargetObjId,
		bIsDead ? 1 : 0,
		*GetNameSafe(Attacker),
		(unsigned long long)AttackerObjId,
		Attacker ? (Attacker->IsMyPlayer() ? 1 : 0) : -1,
		SkillId);

	if (Attacker == nullptr)
		return;

	if (Attacker->IsMyPlayer() == false)
		return;

	if (TargetObjId == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[OnHitBySkill] BLOCK target objId is 0 Target=%s"),
			*GetName());
		return;
	}

	if (TargetObjId == AttackerObjId)
	{
		UE_LOG(LogTemp, Warning, TEXT("[OnHitBySkill] BLOCK self hit ObjId=%llu"),
			(unsigned long long)TargetObjId);
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[OnHitBySkill] SendAttackPlayer target=%llu"),
		(unsigned long long)TargetObjId);

	if (UMainGameInstance* GI = GetGameInstance<UMainGameInstance>())
	{
		GI->SendAttackPlayer(TargetObjId, SkillId);

		UE_LOG(LogTemp, Warning, TEXT("[Client] Hit Player -> SendAttack target=%llu"),
			(unsigned long long)TargetObjId);
	}
}

void APlayerCharacter::SetPlayerInfo(const Protocol::PlayerInfo& Info)
{
	PlayerInfo = Info;
	DestInfo = Info;
}

void APlayerCharacter::SetDestInfo(const Protocol::PlayerInfo& Info)
{
	DestInfo = Info;
}

void APlayerCharacter::SetDead(bool bDead)
{
	UE_LOG(LogTemp, Warning, TEXT("[SetDead ENTER] Actor=%s ObjId=%llu bDead=%d CurrentDead=%d IsMine=%d"),
		*GetNameSafe(this),
		(unsigned long long)PlayerInfo.object_id(),
		bDead ? 1 : 0,
		bIsDead ? 1 : 0,
		bIsMine ? 1 : 0);

	// 이미 같은 상태일 때
	if (bIsDead == bDead)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SetDead SAME STATE] Actor=%s ObjId=%llu bDead=%d CurrentDead=%d"),
			*GetNameSafe(this),
			(unsigned long long)PlayerInfo.object_id(),
			bDead ? 1 : 0,
			bIsDead ? 1 : 0);

		// 중요:
		// 이미 죽은 상태라도 서버에서 죽음 패킷이 다시 왔다면
		// 죽음 애니메이션은 다시 재생해준다.
		if (bDead)
		{
			UE_LOG(LogTemp, Warning, TEXT("[SetDead FORCE DEATH ANIM] Actor=%s ObjId=%llu"),
				*GetNameSafe(this),
				(unsigned long long)PlayerInfo.object_id());

			if (GetCharacterMovement())
			{
				GetCharacterMovement()->StopMovementImmediately();
			}

			PlayDeathAnimation();
		}

		return;
	}

	bIsDead = bDead;

	if (bIsDead)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Client] Player Dead Actor=%s ObjId=%llu"),
			*GetNameSafe(this),
			(unsigned long long)PlayerInfo.object_id());

		if (GetCharacterMovement())
		{
			GetCharacterMovement()->StopMovementImmediately();
		}

		PlayDeathAnimation();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[Client] Player Respawn Actor=%s ObjId=%llu"),
			*GetNameSafe(this),
			(unsigned long long)PlayerInfo.object_id());

		SetActorHiddenInGame(false);
		SetActorEnableCollision(true);

		if (GetCharacterMovement())
		{
			GetCharacterMovement()->SetMovementMode(MOVE_Walking);
			GetCharacterMovement()->Activate(true);
			GetCharacterMovement()->SetComponentTickEnabled(true);
			GetCharacterMovement()->StopMovementImmediately();
		}

		if (GetMesh())
		{
			GetMesh()->SetHiddenInGame(false);
		}

		if (AttributeComponent)
		{
			AttributeComponent->SetHealth(50.f);
		}
	}
}

void APlayerCharacter::Req_UseIceSkill(int32 SkillId, AActor* Target)
{
	LockedTargetActor = Target;

	UE_LOG(LogTemp, Warning, TEXT("[Req_UseIceSkill] Self=%s Target=%s"),
		*GetNameSafe(this),
		*GetNameSafe(LockedTargetActor));

	UMainGameInstance* GI = Cast<UMainGameInstance>(GetGameInstance());
	if (GI == nullptr)
		return;

	GI->SendUseSkill(SkillId, 1);
}

void APlayerCharacter::PlayNetworkAttackAnimation()
{
	UE_LOG(LogTemp, Warning, TEXT("[PlayerCharacter::PlayNetworkAttackAnimation] Called - Implement in Blueprint to play attack animation"));
}

void APlayerCharacter::ApplyNetworkPosition(float DeltaTime)
{
	if (bIsMine)
		return;

	if (bIsDead)
		return;

	FVector TargetPos(DestInfo.x(), DestInfo.y(), DestInfo.z());
	FRotator TargetRot(0.f, DestInfo.yaw(), 0.f);

	FVector CurrentPos = GetActorLocation();
	FVector Vel = (TargetPos - CurrentPos) / DeltaTime;

	GetCharacterMovement()->Velocity = Vel;

	FVector NewPos = FMath::VInterpTo(GetActorLocation(), TargetPos, DeltaTime, 10.f);
	FRotator NewRot = FMath::RInterpTo(GetActorRotation(), TargetRot, DeltaTime, 10.f);

	SetActorLocationAndRotation(NewPos, NewRot);
}

// level transition -> save upgrade
void APlayerCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	/*UMainGameInstance* Instance = Cast<UMainGameInstance>(GetGameInstance());
	Instance->SetLocalPlayerUpgradeMap(UpgradeComponent->GetUpgrades());*/
}