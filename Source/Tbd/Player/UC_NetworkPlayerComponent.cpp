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

    UE_LOG(LogTemp, Warning, TEXT("[UC_NetworkPlayerComponent::BeginPlay] Owner=%s"),
        *GetNameSafe(GetOwner()));

    CachedGameInstance = Cast<UMainGameInstance>(GetWorld()->GetGameInstance());
    if (CachedGameInstance == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("UC_NetworkPlayerComponent: GameInstance 캐스팅 실패!"));
        return;
    }

    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (OwnerPawn == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("[UC_NetworkPlayerComponent::BeginPlay] Owner is not Pawn"));
        return;
    }

    if (!OwnerPawn->IsLocallyControlled())
    {
        UE_LOG(LogTemp, Warning, TEXT("[UC_NetworkPlayerComponent::BeginPlay] Skip timer for non-local pawn: %s"),
            *GetNameSafe(OwnerPawn));
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
    AActor* Owner = GetOwner();
    APawn* OwnerPawn = Cast<APawn>(Owner);

    UE_LOG(LogTemp, Warning, TEXT("[SendMyMovement] ENTER Comp=%s Owner=%s"),
        *GetNameSafe(this),
        *GetNameSafe(Owner));

    if (CachedGameInstance == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("[SendMyMovement] CachedGameInstance nullptr"));
        return;
    }

    if (CachedGameInstance->MyObjectId == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SendMyMovement] GameInstance MyObjectId is 0"));
        return;
    }

    if (OwnerPawn == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("[SendMyMovement] OwnerPawn nullptr"));
        return;
    }

    if (!OwnerPawn->IsLocallyControlled())
    {
        return;
    }

    FVector Location = Owner->GetActorLocation();
    float Yaw = Owner->GetActorRotation().Yaw;

    SendMoveToServer(Location, Yaw);
}

void UC_NetworkPlayerComponent::SendMoveToServer(const FVector& Location, float Yaw)
{
    if (CachedGameInstance == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("[SendMoveToServer] CachedGameInstance nullptr"));
        return;
    }

    const uint64 SendObjectId = CachedGameInstance->MyObjectId;
    if (SendObjectId == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SendMoveToServer] GameInstance MyObjectId not set"));
        return;
    }

    Protocol::C_MOVE MovePkt;
    Protocol::PlayerInfo* Info = MovePkt.mutable_info();

    Info->set_object_id(SendObjectId);
    Info->set_x(Location.X);
    Info->set_y(Location.Y);
    Info->set_z(Location.Z);
    Info->set_yaw(Yaw);

    UE_LOG(LogTemp, Warning, TEXT("[SendMoveToServer] SEND objId=%llu"), SendObjectId);

    SEND_PACKET(MovePkt);
}