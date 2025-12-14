// Fill out your copyright notice in the Description page of Project Settings.


#include "AISpawnManager.h"


// Sets default values
AAISpawnManager::AAISpawnManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	


}

// Called when the game starts or when spawned
void AAISpawnManager::BeginPlay()
{
	Super::BeginPlay();

	World = GetWorld();

	
}

// Called every frame
void AAISpawnManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

bool AAISpawnManager::GenerateRegion() {

	LandscapeActor = UGameplayStatics::GetActorOfClass(GetWorld(), ALandscape::StaticClass());
	LandscapeActor->GetActorBounds(false, LandscapeOrigin, LandscapeExtent);

	LandscapeSizeX = LandscapeExtent.X * 2;
	LandscapeSizeY = LandscapeExtent.Y * 2;

	RegionSizeX = LandscapeSizeX / RegionCountX;
	RegionSizeY = LandscapeSizeY / RegionCountY;

	

	FVector2D LandscapeLocationMin = FVector2D(LandscapeOrigin.X - LandscapeExtent.X, LandscapeOrigin.Y - LandscapeExtent.Y);

	for (int i = 0; i < RegionCountX; i++) {
		for (int j = 0; j < RegionCountY; j++) {
			FFloatRange RangeX = FFloatRange::Inclusive(LandscapeLocationMin.X + i * RegionSizeX, LandscapeLocationMin.X + (i + 1) * RegionSizeX);
			FFloatRange RangeY = FFloatRange::Inclusive(LandscapeLocationMin.Y + j * RegionSizeY, LandscapeLocationMin.Y + (j + 1) * RegionSizeY);
			Regions.Add(FRegionData(RangeX, RangeY));
		}
	}

	return (Regions.Num() == RegionCountX * RegionCountY);

}

bool AAISpawnManager::SpawnEnemies(const ECollisionChannel LandscapeChannel) {
	
	float PointX{ 0 }, PointY{ 0 }, PointZ{ 0 };

	int Total = 0;
	int RegionTotal = 0;

	for (auto& R : Regions) {
		for (int i = R.CurrentEnemyCount; i < R.MaxEnemyCount; i++) {
			PointX = FMath::FRandRange(R.RangeX.GetLowerBoundValue(), R.RangeX.GetUpperBoundValue());
			PointY = FMath::FRandRange(R.RangeY.GetLowerBoundValue(), R.RangeY.GetUpperBoundValue());

			FHitResult HitResult;

			bool Hit = World->UWorld::LineTraceSingleByChannel(
				HitResult,
				FVector(PointX, PointY, LandscapeOrigin.Z + 100000.f),
				FVector(PointX, PointY, LandscapeOrigin.Z - 100000.f),
				LandscapeChannel
			);

			if (!Hit) continue;

			PointZ = HitResult.ImpactPoint.Z;

			AActor* SpawnedAI = World->SpawnActor<AActor>(
				AIClasses[SelectAIClassIndex()],
				FVector(PointX, PointY, PointZ + 1000.f),
				FRotator(0.0f, FMath::FRandRange(0.f, 360.f), 0.0f)
			);
			

			if (SpawnedAI) {
				SpawnedAI->AddActorWorldOffset(FVector(0.0f, 0.0f, -2000.0f), true);
				DisableEnemy(SpawnedAI);
				R.SpawnedEnemies.Add(SpawnedAI);
				R.CurrentEnemyCount++;
				TotalEnemyCount++;
				RegionTotal++;
				Total++;
			}
			
		}
	}

	return (Total == RegionTotal);

}

void AAISpawnManager::DisableAllEnemies() {
	for (auto& R : Regions) {
		for (auto& E : R.SpawnedEnemies) {
			E.Get()->SetActorHiddenInGame(true);
			E.Get()->SetActorEnableCollision(false);
			E.Get()->SetActorTickEnabled(false);
			E.Get()->FindComponentByClass<UCharacterMovementComponent>()->StopMovementImmediately();
			E.Get()->FindComponentByClass<UCharacterMovementComponent>()->SetComponentTickEnabled(false);
			E.Get()->FindComponentByClass<UCharacterMovementComponent>()->DisableMovement();

			//E->DisableComponentsSimulatePhysics();
		}
	}
}

void AAISpawnManager::EnableAllEnemies() {
	for (auto& R : Regions) {
		for (auto& E : R.SpawnedEnemies) {
			E.Get()->SetActorHiddenInGame(false);
			E.Get()->SetActorEnableCollision(true);
			E.Get()->SetActorTickEnabled(true);
			E.Get()->FindComponentByClass<UCharacterMovementComponent>()->SetComponentTickEnabled(true);
			E.Get()->FindComponentByClass<UCharacterMovementComponent>()->SetMovementMode(MOVE_Walking);
			//E->EnableComponentsSimulatePhysics();
		}
	}
}

// AI Ȱ��ȭ
// �ϵ� ��� ���� �÷��̾ ������ ���� Ȱ��ȭ
// ����Ʈ ��� ���� �÷��̾ ������ �Ϻ� Ȱ��ȭ

void AAISpawnManager::UpdateRegionActivation() {

	if (PlayerActors.IsEmpty()) return;

	for (int i = 0; i < Regions.Num(); i++) {
		Regions[i].HardActivation = false;
		Regions[i].SoftActivation = false;
	}
	
	for (const auto& Player : PlayerActors) {
		//if (WeakPlayer.IsValid()) if (AActor* Player = WeakPlayer.Get())
		const FVector PlayerLocation = Player->GetActorLocation();

		// �ϵ� ��� �˻�
		for (int i = 0; i < Regions.Num(); i++) {
			if (Regions[i].RangeX.GetLowerBoundValue() < PlayerLocation.X &&
				PlayerLocation.X < Regions[i].RangeX.GetUpperBoundValue() &&
				Regions[i].RangeY.GetLowerBoundValue() < PlayerLocation.Y &&
				PlayerLocation.Y < Regions[i].RangeY.GetUpperBoundValue())
			{
				Regions[i].HardActivation = true;
				for (auto& WeakE : Regions[i].SpawnedEnemies) {
					// �ϵ� Ȱ��ȭ
					if (WeakE.IsValid()) { if (AActor* E = WeakE.Get()) EnableEnemy(E); }
				}

				const int CoreRegionIndexX = i / RegionCountY;
				const int CoreRegionIndexY = i % RegionCountY;

				// ����Ʈ ��� �˻�
				for (int di = -1; di <= 1; di++) {
					for (int dj = -1; dj <= 1; dj++) {

						if (di == 0 && dj == 0) continue;

						const int NeighborIndexX = CoreRegionIndexX + di;
						const int NeighborIndexY = CoreRegionIndexY + dj;

						if (0 <= NeighborIndexX && NeighborIndexX < RegionCountX &&
							0 <= NeighborIndexY && NeighborIndexY < RegionCountY)
						{
							const int NeighborIndex = NeighborIndexX * RegionCountY + NeighborIndexY;
							Regions[NeighborIndex].SoftActivation = true;

							// ����Ʈ Ȱ��ȭ
							// �ٵ� ����Ʈ Ȱ��ȭ �ϱ� ���� �ϵ� Ȱ��ȭ�� �̹� �Ǿ��ִ� �� �˻��ؾ���
							// �Լ�
							if (!Regions[NeighborIndex].HardActivation) {
								for (auto& WeakE : Regions[NeighborIndex].SpawnedEnemies) {
									if (WeakE.IsValid()) { if (AActor* E = WeakE.Get()) EnableEnemy(E); }
								}
							} 
						}

					}
				}

			}
		}

	}

	for (int i = 0; i < Regions.Num(); i++) {
		if (!Regions[i].HardActivation && !Regions[i].SoftActivation) {
			for (auto& WeakE : Regions[i].SpawnedEnemies) {
				if (WeakE.IsValid()) { if (AActor* E = WeakE.Get()) DisableEnemy(E); }
			}
		}
	}

}

void AAISpawnManager::Update() {

	SpawnEnemies(LandscapeCollisionChannel);

	UpdateRegionActivation();

}

FTimerHandle& AAISpawnManager::SetUpdateTimer(FTimerHandle& TimerHandle) {

	World->GetTimerManager().SetTimer(
		TimerHandle,
		this,
		&AAISpawnManager::Update,
		5.0f,
		true,
		1.0f
	);

	return TimerHandle;
}

void AAISpawnManager::EndPlay(const EEndPlayReason::Type EndPlayReason) {

	Super::EndPlay(EndPlayReason);

	UpdateTimerHandle.Invalidate();
	
}