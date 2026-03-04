// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "NPC_AIController.generated.h"

/**
 * 
 */
UCLASS()
class TBD_API ANPC_AIController : public AAIController
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "NPC_AIController")
	void UpdateEnable();
	virtual void UpdateEnable_Implementation() {};

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "NPC_AIController")
	void UpdateDisable();
	virtual void UpdateDisable_Implementation() {};
	//explicit ANPC_AIController(FObjectInitializer const& ObjectInitializer);
};
