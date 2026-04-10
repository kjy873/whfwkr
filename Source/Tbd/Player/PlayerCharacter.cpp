#include "Tbd/Player/PlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "MainGameInstance.h"

// Sets default values
APlayerCharacter::APlayerCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

APlayerCharacter::~APlayerCharacter()
{
}

// Called when the game starts or when spawned
void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocallyControlled())
	{
		if (UMainGameInstance* GI = GetGameInstance<UMainGameInstance>())
		{
			GI->SendLevelReady();
			UE_LOG(LogTemp, Warning, TEXT("[Client] SendLevelReady from BeginPlay"));
		}
	}
}

// Called every frame

void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	ApplyNetworkPosition(DeltaTime);
}

void APlayerCharacter::OnHitBySkill(APlayerCharacter* Attacker, int32 SkillId)
{
	if (Attacker == nullptr)
		return;

	if (Attacker->IsMyPlayer() == false)
		return;

	if (UMainGameInstance* GI = GetGameInstance<UMainGameInstance>())
	{
		GI->SendAttackPlayer(PlayerInfo.object_id(), SkillId);

		UE_LOG(LogTemp, Warning, TEXT("[Client] Hit Player -> SendAttack target=%llu"),
			PlayerInfo.object_id());
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
	UE_LOG(LogTemp, Warning, TEXT("[SetDead] %s bDead=%d IsMine=%d"),
		*GetName(), bDead ? 1 : 0, bIsMine ? 1 : 0);

	if (bIsDead == bDead)
		return;

	bIsDead = bDead;

	if (bIsDead)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Client] Player Dead"));

		if (GetCharacterMovement())
		{
			GetCharacterMovement()->StopMovementImmediately();
			GetCharacterMovement()->DisableMovement();
		}

		PlayDeathAnimation();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[Client] Player Respawn"));

		if (GetCharacterMovement())
		{
			GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		}
	}
}

void APlayerCharacter::Req_UseIceSkill(int32 SkillId, AActor* Target)
{
	uint64 TargetId = 0;

	if (Target)
	{
		APlayerCharacter* TargetPlayer = Cast<APlayerCharacter>(Target);
		if (TargetPlayer)
		{
			TargetId = TargetPlayer->PlayerInfo.object_id();
		}
	}

	Protocol::C_USE_SKILL pkt;
	pkt.set_skillid(SkillId);
	pkt.set_targetid(TargetId);

	UMainGameInstance* GI = Cast<UMainGameInstance>(GetGameInstance());
	if (GI)
	{
		auto sendBuffer = ClientPacketHandler::MakeSendBuffer(pkt);
		GI->SendPacket(sendBuffer);
	}
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

