#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

class USceneComponent;
class USphereComponent;
class UBoxComponent;
class UProjectileMovementComponent;
class APlayerCharacter;

UCLASS()
class TBD_API AProjectile : public AActor
{
	GENERATED_BODY()

public:
	AProjectile();

	void SetProjectileInfo(APlayerCharacter* InOwnerPlayer, int32 InSkillId);

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
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<USceneComponent> SceneRoot;

	// 스피어 1
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<USphereComponent> SphereCollision1;

	// 스피어 2
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<USphereComponent> SphereCollision2;

	// 박스
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UBoxComponent> BoxCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMove;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	float Damage = 10.f;

	UPROPERTY()
	TObjectPtr<APlayerCharacter> OwnerPlayer;

	UPROPERTY()
	int32 SkillId = 0;

	UPROPERTY()
	bool bHasHit = false;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Projectile")
	void DebugFunc();
	virtual void DebugFunc_Implementation() {};
};