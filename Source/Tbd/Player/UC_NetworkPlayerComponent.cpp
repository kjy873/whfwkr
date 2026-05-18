#include "Player/UC_NetworkPlayerComponent.h"
#include "MainGameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"

UC_NetworkPlayerComponent::UC_NetworkPlayerComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UC_NetworkPlayerComponent::BeginPlay()
{
    Super::BeginPlay();

    CachedGameInstance = Cast<UMainGameInstance>(GetWorld()->GetGameInstance());
}

void UC_NetworkPlayerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    StopMoveSendTimer();

    Super::EndPlay(EndPlayReason);
}

void UC_NetworkPlayerComponent::StartMoveSendTimer()
{
    if (GetWorld() == nullptr)
        return;

    if (CachedGameInstance == nullptr)
    {
        CachedGameInstance = Cast<UMainGameInstance>(GetWorld()->GetGameInstance());
    }

    if (CachedGameInstance == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("[StartMoveSendTimer] CachedGameInstance is null"));
        return;
    }

    if (CachedGameInstance->MyObjectId == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[StartMoveSendTimer] BLOCK MyObjectId is 0 Owner=%s"),
            *GetNameSafe(GetOwner()));
        return;
    }

    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (OwnerPawn == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("[StartMoveSendTimer] OwnerPawn is null Owner=%s"),
            *GetNameSafe(GetOwner()));
        return;
    }

    if (!OwnerPawn->IsLocallyControlled())
    {
        UE_LOG(LogTemp, Warning, TEXT("[StartMoveSendTimer] Not locally controlled: %s"),
            *GetNameSafe(OwnerPawn));
        return;
    }

    if (GetWorld()->GetTimerManager().IsTimerActive(MoveSendTimer))
    {
        UE_LOG(LogTemp, Warning, TEXT("[StartMoveSendTimer] Timer already active: %s"),
            *GetNameSafe(OwnerPawn));
        return;
    }

    bHasSentInitialMove = false;
    LastSentLocation = FVector::ZeroVector;
    LastSentYaw = 0.f;

    GetWorld()->GetTimerManager().SetTimer(
        MoveSendTimer,
        this,
        &UC_NetworkPlayerComponent::SendMyMovement,
        MoveSendInterval,
        true
    );

    UE_LOG(LogTemp, Warning, TEXT("[StartMoveSendTimer] Timer started for local pawn: %s ObjectId=%llu"),
        *GetNameSafe(OwnerPawn),
        (unsigned long long)CachedGameInstance->MyObjectId);
}

void UC_NetworkPlayerComponent::StopMoveSendTimer()
{
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(MoveSendTimer);
    }

    bHasSentInitialMove = false;

    UE_LOG(LogTemp, Warning, TEXT("[StopMoveSendTimer] Timer stopped Owner=%s"),
        *GetNameSafe(GetOwner()));
}

void UC_NetworkPlayerComponent::SendMyMovement()
{
    if (CachedGameInstance == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SendMyMovement] CachedGameInstance is null"));
        return;
    }

    const uint64 ObjectId = CachedGameInstance->MyObjectId;
    if (ObjectId == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SendMyMovement] MyObjectId is 0"));
        return;
    }

    AActor* Owner = GetOwner();
    if (Owner == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SendMyMovement] Owner is null"));
        return;
    }

    APawn* OwnerPawn = Cast<APawn>(Owner);
    if (OwnerPawn && !OwnerPawn->IsLocallyControlled())
    {
        UE_LOG(LogTemp, Warning, TEXT("[SendMyMovement] Not locally controlled: %s"),
            *GetNameSafe(OwnerPawn));
        return;
    }

    const FVector Location = Owner->GetActorLocation();
    const float Yaw = Owner->GetActorRotation().Yaw;

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

    UE_LOG(LogTemp, Warning, TEXT("[SendMyMovement] Send ObjId=%llu Loc=%s Yaw=%f"),
        (unsigned long long)ObjectId,
        *Location.ToString(),
        Yaw);

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