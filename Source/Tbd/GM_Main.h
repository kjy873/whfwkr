// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "RenderingThread.h"
#include "GameFramework/GameMode.h"
#include "GM_Main.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class TBD_API AGM_Main : public AGameMode
{
	GENERATED_BODY()
	
public:

	virtual void BeginPlay() override;
	//virtual void PostInitializeComponents() override;

};
