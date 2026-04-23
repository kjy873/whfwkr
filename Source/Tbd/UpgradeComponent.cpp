// Fill out your copyright notice in the Description page of Project Settings.


#include "UpgradeComponent.h"
#include "Algo/RandomShuffle.h"

// Sets default values for this component's properties
UUpgradeComponent::UUpgradeComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UUpgradeComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UUpgradeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

// 랜덤 3개 업그레이드 반환 함수
// Available에서 3개 뽑아서 검사
// 1. 등장 가능 업그레이드 & 이미 보유한 업그레이드 & 재등장 가능 = 추가
// 2. 등장 가능 업그레이드 & 보유하지 않은 업그레이드 = 추가
// == 보유하지 않았거나 재등장 가능하면 추가

TArray<FName> UUpgradeComponent::Make3Combination() {
	
	TArray<FName> Result;

	TArray<FName> Candidates = AvailableUpgrades.Array();
	Algo::RandomShuffle(Candidates);

	for (const FName& Candidate : Candidates) {
		if (Result.Num() >= 3) break;
		if (!HasUpgrade(Candidate) || ReusableUpgrades.Contains(Candidate)) Result.Add(Candidate);
	
	}

	ensureMsgf(Result.Num() == 3, TEXT("Failed to generate 3 upgrades"));
	return Result;

}