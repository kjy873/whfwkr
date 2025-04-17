// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Landscape.h"
#include "LandscapeEdit.h"
#include "LandscapeInfo.h"
#include "EngineUtils.h"
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PCGFunctionLibrary.generated.h"

/**
 *
 */
UCLASS()
class TBD_API UPCGFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "PCG")
	static ALandscape* FindLandscape(UWorld* world);

	UFUNCTION(BlueprintCallable, Category = "PCG")
	static void ApplyHeightMap(ALandscape* Landscape, TArray<int32>& HeightMap, int32 Width, int32 Height);

	UFUNCTION(BlueprintCallable, Category = "PCG")
	static void GenerateHeightMap(TArray<int32>& HeightMap, int32& Width, int32& Height, float Scale);

	UFUNCTION(BlueprintCallable, Category = "PCG")
	static void GenerateMHeightMap(TArray<int32>& HeightMap, int32& Width, int32& Height, float Scale);

	UFUNCTION(BlueprintCallable, Category = "PCG")
	static void GetLandscapeSize(ALandscape* Landscape, int32& VerticesX, int32& VerticesY);

	UFUNCTION(BlueprintCallable, Category = "PCG")
	static float GetHeight(ALandscape* Landscape, float x, float y);
};
