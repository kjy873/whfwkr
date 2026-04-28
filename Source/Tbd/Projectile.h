#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

class UProjectileMovementComponent;
class USphereComponent;
class USceneComponent;
class UBoxComponent;
class APlayerCharacter;

UCLASS()
class TBD_API AProjectile : public AActor
{
    GENERATED_BODY()

public:
    AProjectile();
    void SetProjectileInfo(APlayerCharacter* InOwnerPlayer, int32 InSkillId);
    void ActivateProjectileCollision();
    void LaunchProjectile(FVector Direction);
    void SetChargeScale(float InScale);

protected:
    virtual void BeginPlay() override;

    UFUNCTION()
    void OnProjectileOverlap(
        UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult
    );

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<USphereComponent> SphereCollision1;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<USphereComponent> SphereCollision2;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<UBoxComponent> BoxCollision;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<UProjectileMovementComponent> ProjectileMove;

    UPROPERTY()
    APlayerCharacter* OwnerPlayer = nullptr;

    int32 SkillId = 0;
    bool bHasHit = false;
    bool bLaunched = false;

    float BaseSphere1Radius = 0.f;
    float BaseSphere2Radius = 0.f;
};