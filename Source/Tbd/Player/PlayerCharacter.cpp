#include "Tbd/Player/PlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

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
}

// Called every frame

void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	ApplyNetworkPosition(DeltaTime);
}

void APlayerCharacter::SetPlayerInfo(const Protocol::PlayerInfo& Info)
{
	PlayerInfo = Info;
	DestInfo = Info;

	FVector Pos(Info.x(), Info.y(), Info.z());
	FRotator Rot(0.f, Info.yaw(), 0.f);
	SetActorLocationAndRotation(Pos, Rot);

}

void APlayerCharacter::SetDestInfo(const Protocol::PlayerInfo& Info)
{
	DestInfo = Info;
}

void APlayerCharacter::ApplyNetworkPosition(float DeltaTime)
{
	if (bIsMine)
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
