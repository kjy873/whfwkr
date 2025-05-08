// Fill out your copyright notice in the Description page of Project Settings.


#include "PCGFunctionLibrary.h"

ALandscape* UPCGFunctionLibrary::FindLandscape(UWorld* world) {
	if (!world) {
		UE_LOG(LogTemp, Warning, TEXT("World is null"));
	}

	for (TActorIterator<ALandscape> It(world); It; ++It) {
		ALandscape* Landscape = *It;
		if (Landscape) {
			UE_LOG(LogTemp, Warning, TEXT("Found Landscape : %s"), *Landscape->GetName());
			return Landscape;
		}
	}

	return nullptr;
}

//void UPCGFunctionLibrary::ApplyHeightMap(ALandscape* Landscape, TArray<int32>& HeightMap, int32 Width, int32 Height) {
//
//	if (!Landscape)
//	{
//		UE_LOG(LogTemp, Error, TEXT("ApplyHeightMap: Landscape is NULL!"));
//		return;
//	}
//
//	TArray<uint16> HeightMapUint16;
//	HeightMapUint16.SetNum(HeightMap.Num());
//
//	for (int32 i = 0; i < HeightMap.Num(); i++)
//	{
//		HeightMapUint16[i] = static_cast<uint16>(FMath::Clamp(HeightMap[i], 0, 65535));  // uint16 범위로 변환
//	}
//
//
//	if (!Landscape || HeightMap.Num() == 0)
//	{
//		UE_LOG(LogTemp, Warning, TEXT("Invalid Landscape or HeightMap data!"));
//		return;
//	}
//
//	ULandscapeInfo* LandscapeInfo = Landscape->GetLandscapeInfo();
//
//	if (!LandscapeInfo)
//	{
//		UE_LOG(LogTemp, Error, TEXT("ApplyHeightMap: LandscapeInfo is NULL!"));
//		return;
//	}
//
//	FLandscapeEditDataInterface LandscapeEditer(LandscapeInfo);
//	LandscapeEditer.SetHeightData(0, 0, Width - 1, Height - 1, HeightMapUint16.GetData(), 0, true);
//	LandscapeEditer.Flush();
//	Landscape->MarkPackageDirty();
//	//Landscape->UpdateAllComponentMaterialInstances();
//	//Landscape->FlushGrassComponents();
//	//Landscape->RecreateCollisionComponents();
//	Landscape->RecreateCollisionComponents();
//}

void UPCGFunctionLibrary::GenerateHeightMap(TArray<int32>& HeightMap, int32& Width, int32& Height, float Scale) {
	HeightMap.SetNum(Width * Height);
	for (int32 y = 0; y < Height; y++)
	{
		for (int32 x = 0; x < Width; x++)
		{
			int32 Index = y * Width + x;
			FVector2D NoiseInput(x * Scale, y * Scale);
			float NoiseValue = FMath::PerlinNoise2D(NoiseInput);
			NoiseValue = (NoiseValue + 1.0f) * 0.5f;
			HeightMap[Index] = static_cast<uint16>(NoiseValue * 65535);
		}
	}
}

void UPCGFunctionLibrary::GenerateMHeightMap(TArray<int32>& HeightMap, int32& Width, int32& Height, float Scale) {
	HeightMap.SetNum(Width * Height);
	for (int32 y = 0; y < Height; y++)
	{
		for (int32 x = 0; x < Width; x++)
		{
			int32 Index = y * Width + x;
			FVector2D NoiseInput(x * Scale, y * Scale);
			float NoiseValue = 0.5 * FMath::PerlinNoise2D(NoiseInput) + 0.25 * FMath::PerlinNoise2D(2 * NoiseInput) + 0.25 * FMath::PerlinNoise2D(4 * NoiseInput);
			NoiseValue = (NoiseValue + 1.0f) * 0.5f;
			HeightMap[Index] = static_cast<uint16>(NoiseValue * 65535);
		}
	}
}

// 랜드스케이프의 x, y 버텍스(높이 맵 배열 선언용)
void UPCGFunctionLibrary::GetLandscapeSize(ALandscape* Landscape, int32& Xcounter, int32& Ycounter) {

	int32 SectionSize = Landscape->ComponentSizeQuads;
	int32 SubSectionComponent = Landscape->NumSubsections;
	int32 ComponentsX = Landscape->LandscapeComponents.Num();
	int32 ComponentsY = ComponentsX;

	Xcounter = (SectionSize + 1) * SubSectionComponent * ComponentsX;
	Ycounter = (SectionSize + 1) * SubSectionComponent * ComponentsY;

}

float UPCGFunctionLibrary::GetHeight(ALandscape* Landscape, float x, float y) {
	if (!Landscape) return 0.0f;
	return Landscape->GetHeightAtLocation(FVector(x, y, 0.0f)).GetValue();
}

