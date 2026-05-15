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

    UFUNCTION(BlueprintCallable)
    void SetProjectileInfo(int32 InSkillId, AActor* InOwnerPlayer);

    UFUNCTION(BlueprintCallable)
    void LaunchProjectile(const FVector& Direction);

    void SetChargeScale(float InScale);

    virtual void Tick(float DeltaTime) override;

    void CheckManualOverlap();
    void HandleProjectileHit(AActor* OtherActor);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
    float HitCheckRadius = 80.f;

    UFUNCTION(BlueprintCallable)
    void ActivateProjectileCollision();

    UPROPERTY()
    FVector MoveDirection = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MoveSpeed = 2000.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Homing")
    AActor* HomingTarget = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Homing")
    bool bUseHoming = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Homing")
    float HomingInterpSpeed = 3.0f;

    void SetHomingTarget(AActor* Target);

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

	UFUNCTION(BlueprintImplementableEvent)
    void PostProcessHit();



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
    AActor* OwnerPlayer = nullptr;

    int32 SkillId = 0;
    bool bHasHit = false;
    bool bLaunched = false;

    float BaseSphere1Radius = 0.f;
    float BaseSphere2Radius = 0.f;
};