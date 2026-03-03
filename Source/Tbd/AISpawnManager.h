// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Landscape.h"
#include "NPC_AIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Character.h"
#include "Runtime/Engine/Classes/Kismet/GameplayStatics.h"
#include "AISpawnManager.generated.h"

USTRUCT(BlueprintType)
struct FRegionData
{
	GENERATED_BODY()

	FFloatRange RangeX;
	FFloatRange RangeY;

	bool HardActivation = false;
	bool SoftActivation = false;

	int MaxEnemyCount;

	int CurrentEnemyCount;

	
	TArray<TWeakObjectPtr<AActor>> SpawnedEnemies;

	FRegionData() : RangeX(FFloatRange::Empty()), RangeY(FFloatRange::Empty()), MaxEnemyCount(1), CurrentEnemyCount(0) {}

	FRegionData(FFloatRange x, FFloatRange y) : RangeX(x), RangeY(y), MaxEnemyCount(1), CurrentEnemyCount(0) {}

	FRegionData(FFloatRange x, FFloatRange y, int maxCount) : RangeX(x), RangeY(y), MaxEnemyCount(maxCount), CurrentEnemyCount(0) {}
};

UCLASS(BlueprintType)
class TBD_API AAISpawnManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AAISpawnManager();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AISpawnManager")
	UWorld* World = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AISpawnManager")
	AActor* LandscapeActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AISpawnManager")
	TArray<FRegionData> Regions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AISpawnManager")
	int TotalEnemyCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AISpawnManager")
	FVector LandscapeOrigin = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AISpawnManager")
	FVector LandscapeExtent = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AISpawnManager")
	float LandscapeSizeX = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AISpawnManager")
	float LandscapeSizeY = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AISpawnManager")
	int RegionCountX = 8;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AISpawnManager")
	int RegionCountY = 8;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AISpawnManager")
	float RegionSizeX = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AISpawnManager")
	float RegionSizeY = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AISpawnManager")
	TArray<TSubclassOf<AActor>> AIClasses;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AISpawnManager")
	TArray<AActor*> PlayerActors;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AISpawnManager")
	FTimerHandle UpdateTimerHandle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AISpawnManager")
	int DebugCounter = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AISpawnManager")
	TEnumAsByte<ECollisionChannel> LandscapeCollisionChannel;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "AISpawnManager")
	bool GenerateRegion();

	UFUNCTION(BlueprintCallable, Category = "AISpawnManager")
	bool SpawnEnemies(const ECollisionChannel LandscapeChannel);

	UFUNCTION(BlueprintCallable, Category = "AISpawnManager")
	int SelectAIClassIndex() { return FMath::RandRange(0, AIClasses.Num() - 1); }

	UFUNCTION(BlueprintCallable, Category = "AISpawnManager")
	void DisableAllEnemies();

	UFUNCTION(BlueprintCallable, Category = "AISpawnManager")
	void EnableAllEnemies();

	UFUNCTION(BlueprintCallable, Category = "AISpawnManager")
	void GetWorldPlayers(UWorld* WorldActor, TArray<AActor*>& PlayerArray, TSubclassOf<AActor> CharacterClass) {
		UGameplayStatics::GetAllActorsOfClass(WorldActor, CharacterClass, PlayerArray);
	}

	UFUNCTION(BlueprintCallable, Category = "AISpawnManager")
	void UpdateRegionActivation();

	UFUNCTION(BlueprintCallable, Category = "AISpawnManager")
	bool GetRegionHardActivation(const int& index) { return Regions[index].HardActivation; }

	UFUNCTION(BlueprintCallable, Category = "AISpawnManager")
	bool GetRegionSoftActivation(const int& index) { return Regions[index].SoftActivation; }

	UFUNCTION(BlueprintCallable, Category = "AISpawnManager")
	void DeleteRegionEnemy(const int& RegionIndex, AActor* EnemyActor) {
		Regions[RegionIndex].SpawnedEnemies.Remove(EnemyActor);
		Regions[RegionIndex].CurrentEnemyCount--;
		TotalEnemyCount--;
	}

	UFUNCTION(BlueprintCallable, Category = "AISpawnManager")
	inline void DisableEnemy(AActor* EnemyActor) {
		EnemyActor->SetActorHiddenInGame(true);
		EnemyActor->SetActorEnableCollision(false);
		EnemyActor->SetActorTickEnabled(false);
		EnemyActor->FindComponentByClass<UCharacterMovementComponent>()->StopMovementImmediately();
		EnemyActor->FindComponentByClass<UCharacterMovementComponent>()->SetComponentTickEnabled(false);
		EnemyActor->FindComponentByClass<UCharacterMovementComponent>()->DisableMovement();

		if (APawn* EnemyPawn = Cast<APawn>(EnemyActor)) {
			if (ANPC_AIController* EnemyController = Cast<ANPC_AIController>(EnemyPawn->GetController())) {
				EnemyController->UpdateDisable();
			}
		}
		//E->DisableComponentsSimulatePhysics();
	}

	UFUNCTION(BlueprintCallable, Category = "AISpawnManager")
	inline void EnableEnemy(AActor* EnemyActor) {
		EnemyActor->SetActorHiddenInGame(false);
		EnemyActor->SetActorEnableCollision(true);
		EnemyActor->SetActorTickEnabled(true);
		EnemyActor->FindComponentByClass<UCharacterMovementComponent>()->SetComponentTickEnabled(true);
		EnemyActor->FindComponentByClass<UCharacterMovementComponent>()->SetMovementMode(MOVE_Walking);

		if (APawn* EnemyPawn = Cast<APawn>(EnemyActor)) {
			if (ANPC_AIController* EnemyController = Cast<ANPC_AIController>(EnemyPawn->GetController())) {
				EnemyController->UpdateEnable();
			}
		}
		//E->EnableComponentsSimulatePhysics();
	}

	UFUNCTION(BlueprintCallable, Category = "AISpawnManager")
	void Update();


	UFUNCTION(BlueprintCallable, Category = "AISpawnManager")
	FTimerHandle& SetUpdateTimer(FTimerHandle& TimerHandle);

	UFUNCTION(BlueprintCallable, Category = "AISpawnManager")
	bool GetCurrentWorld() {
		World = GetWorld();
		return (World != nullptr);
	}
	

};
