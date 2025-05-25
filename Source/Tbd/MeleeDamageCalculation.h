// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "MeleeDamageCalculation.generated.h"

/**
 * 
 */
UCLASS()
class TBD_API UMeleeDamageCalculation : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UMeleeDamageCalculation();

	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, 
										OUT FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "Calculations")
	float GetDamageMagnitude(AActor* SourceActor) const;

	UFUNCTION(BlueprintNativeEvent, Category = "Calculations")
	float GetWeaponDamage(AActor* SourceActor) const;

	UFUNCTION(BlueprintNativeEvent, Category = "Calculations")
	float GetBlockAbsorption(AActor* TargetActor) const;

	UFUNCTION(BlueprintNativeEvent, Category = "Calculations")
	void OnFullDamageAbsorbed(AActor* SourceActor, AActor* TargetActor);

	virtual void OnFullDamageAbsorbed_Implementation(AActor* SourceActor, AActor* TargetActor) const;

	UFUNCTION(BlueprintNativeEvent, Category = "Calculations")
	void OnDamageDone(AActor* SourceActor, AActor* TargetActor, float Damage);

	virtual void OnDamageDone_Implementation(AActor* SourceActor, AActor* TargetActor, float Damage) const;


};
