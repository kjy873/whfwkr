#include "Player/UC_NetworkPlayerComponent.h"
#include "MainGameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

UC_NetworkPlayerComponent::UC_NetworkPlayerComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UC_NetworkPlayerComponent::BeginPlay()
{
    Super::BeginPlay();

    CachedGameInstance = Cast<UMainGameInstance>(GetWorld()->GetGameInstance());
    if (CachedGameInstance == nullptr)
    {
        return;
    }

    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (OwnerPawn == nullptr)
    {
        return;
    }

    if (!OwnerPawn->IsLocallyControlled())
    {
        return;
    }

    GetWorld()->GetTimerManager().SetTimer(
        MoveSendTimer,
        this,
        &UC_NetworkPlayerComponent::SendMyMovement,
        MoveSendInterval,
        true
    );

    UE_LOG(LogTemp, Warning, TEXT("[UC_NetworkPlayerComponent::BeginPlay] Timer started for local pawn: %s"),
        *GetNameSafe(OwnerPawn));
}

void UC_NetworkPlayerComponent::SendMyMovement()
{
    if (!CachedGameInstance)
        return;

    const uint64 ObjectId = CachedGameInstance->MyObjectId;
    if (ObjectId == 0)
        return;

    AActor* Owner = GetOwner();
    if (!Owner)
        return;

    FVector Location = Owner->GetActorLocation();
    float Yaw = Owner->GetActorRotation().Yaw;

    if (bHasSentInitialMove)
    {
        if (FVector::Dist(Location, LastSentLocation) < 10.f &&
            FMath::Abs(Yaw - LastSentYaw) < 2.f)
        {
            return;
        }
    }

    LastSentLocation = Location;
    LastSentYaw = Yaw;
    bHasSentInitialMove = true;

    SendMoveToServer(Location, Yaw);
}

void UC_NetworkPlayerComponent::SendMoveToServer(const FVector& Location, float Yaw)
{
    if (CachedGameInstance == nullptr)
    {
        return;
    }

    const uint64 SendObjectId = CachedGameInstance->MyObjectId;
    if (SendObjectId == 0)
    {
        return;
    }

    Protocol::C_MOVE MovePkt;
    Protocol::PlayerInfo* Info = MovePkt.mutable_info();

    Info->set_object_id(SendObjectId);
    Info->set_x(Location.X);
    Info->set_y(Location.Y);
    Info->set_z(Location.Z);
    Info->set_yaw(Yaw);

    SEND_PACKET(MovePkt);
}