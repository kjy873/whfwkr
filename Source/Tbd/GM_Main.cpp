// Fill out your copyright notice in the Description page of Project Settings.

#include "GM_Main.h"
#include "PCGFunctionLibrary.h"


//void AGM_Main::PostInitializeComponents() {
//	Super::PostInitializeComponents();
//
//}

void AGM_Main::BeginPlay() {
	Super::BeginPlay();

	//UWorld* World = GetWorld();

	//ALandscape* Landscape = nullptr;
	//int32 WidthLV{ 0 }, HeightLV{ 0 };
	//TArray<int32> HeightMap;
	//HeightMap.SetNum(0);
	//if (World) {
	//	UE_LOG(LogTemp, Warning, TEXT("found world. execute LandscapeHeightmapApplier"));

	//	Landscape = UPCGFunctionLibrary::FindLandscape(World);

	//	FVector CurrentScale = Landscape->GetActorScale3D();
	//	Landscape->SetActorScale3D(FVector(CurrentScale.X, CurrentScale.Y, CurrentScale.Z));

	//	UPCGFunctionLibrary::GetLandscapeSize(Landscape, WidthLV, HeightLV);
	//	HeightMap.SetNum(WidthLV * HeightLV);
	//	UPCGFunctionLibrary::GenerateMHeightMap(HeightMap, WidthLV, HeightLV, 0.01f);
	//	//UPCGFunctionLibrary::ErosionSimulator(HeightMap, WidthLV, HeightLV, 100);
	//	UPCGFunctionLibrary::ApplyHeightMap(Landscape, HeightMap, WidthLV, HeightLV);
	//}
}