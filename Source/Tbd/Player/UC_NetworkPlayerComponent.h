#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UC_NetworkPlayerComponent.generated.h"

class UMainGameInstance;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class TBD_API UC_NetworkPlayerComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UC_NetworkPlayerComponent();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    void SetObjectId(uint64 InObjectId) { MyObjectId = InObjectId; }

    UFUNCTION(BlueprintCallable)
    void StartMoveSendTimer();

    UFUNCTION(BlueprintCallable)
    void StopMoveSendTimer();

    UFUNCTION(BlueprintCallable)
    void SendMoveToServer(const FVector& Location, float Yaw);

    void SendMyMovement();

private:
    UPROPERTY()
    UMainGameInstance* CachedGameInstance = nullptr;

    uint64 MyObjectId = 0;

    FTimerHandle MoveSendTimer;

    float MoveSendInterval = 0.2f;

    FVector LastSentLocation = FVector::ZeroVector;
    float LastSentYaw = 0.f;
    bool bHasSentInitialMove = false;
};