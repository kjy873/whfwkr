#include "Tbd/Player/PlayerCharacter.h"

// Sets default values
APlayerCharacter::APlayerCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	PlayerInfo = new Protocol::PlayerInfo();
	DestInfo = new Protocol::PlayerInfo();

}

APlayerCharacter::~APlayerCharacter()
{
	delete PlayerInfo;
	delete DestInfo;
	PlayerInfo = nullptr;
	DestInfo = nullptr;
}

// Called when the game starts or when spawned
void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame

void APlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

bool APlayerCharacter::IsMyPlayer()
{
	return false;
}

void APlayerCharacter::SetPlayerInfo(const Protocol::PlayerInfo& Info)
{
}

void APlayerCharacter::SetDestInfo(const Protocol::PlayerInfo& Info)
{
}
