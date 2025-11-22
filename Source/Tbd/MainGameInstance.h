// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "MainGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class TBD_API UMainGameInstance : public UGameInstance
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Settings")
	int LandscapeMaterialQualityLevel = 0;

	UFUNCTION(BlueprintCallable, Category = "Game Settings")
	int GetLandscapeMaterialQualityLevel() const { return LandscapeMaterialQualityLevel; }

	UFUNCTION(BlueprintCallable, Category = "Game Settings")
	void SetLandscapeMaterialQualityLevel(int NewQualityLevel) { LandscapeMaterialQualityLevel = NewQualityLevel; }
};
