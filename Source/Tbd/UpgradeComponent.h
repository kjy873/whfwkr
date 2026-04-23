// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Attributes.h"
#include "Components/ActorComponent.h"
#include "UpgradeComponent.generated.h"


UCLASS( Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TBD_API UUpgradeComponent : public UActorComponent
{
	GENERATED_BODY()
	
	// 보유 업그레이드
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade", meta = (AllowPrivateAccess = "true"))
	FPlayerUpgradeState UpgradeState;


	// 등장 가능 업그레이드, 재등장 가능 업그레이드. 두 Set는 BP 에디터에서 직접 Default 설정
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade", meta = (AllowPrivateAccess = "true"))
	TSet<FName> AvailableUpgrades;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade", meta = (AllowPrivateAccess = "true"))
	TSet<FName> ReusableUpgrades;


public:	
	// Sets default values for this component's properties
	UUpgradeComponent();

	UFUNCTION(BlueprintCallable, Category = "Upgrade")
	void AddUpgrade(const FName& UpgradeName) { 
		ensureMsgf(AvailableUpgrades.Contains(UpgradeName), TEXT("Invalid Upgrade: %s"), *UpgradeName.ToString()); 
		UpgradeState.AddUpgrade(UpgradeName); 
	}

	UFUNCTION(BlueprintCallable, Category = "Upgrade")
	bool HasUpgrade(const FName& UpgradeName) const { return UpgradeState.HasUpgrade(UpgradeName); }

	UFUNCTION(BlueprintCallable, Category = "Upgrade")
	void SetupUpgrades(const TMap<FName, int>& Src) { UpgradeState.SetUpgrades(Src); }

	UFUNCTION(BlueprintCallable, Category = "Upgrade")
	TMap<FName, int> GetUpgrades() const { return UpgradeState.AquiredUpgrades; }

	UFUNCTION(BlueprintCallable, Category = "Upgrade")
	int GetUpgradeCount(const FName& UpgradeName) const { return UpgradeState.GetUpgradeCount(UpgradeName); }

	UFUNCTION(BlueprintCallable, Category = "Upgrade")
	TArray<FName> Make3Combination();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};
