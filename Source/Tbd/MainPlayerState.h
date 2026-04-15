// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Attributes.h"
#include "GameFramework/PlayerState.h"
#include "MainPlayerState.generated.h"



/**
 * 
 */
UCLASS()
class TBD_API AMainPlayerState : public APlayerState
{
	GENERATED_BODY()


private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attributes", meta=(AllowPrivateAccess="true"))
	FAttribute Attributes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unlocks", meta = (AllowPrivateAccess = "true"))
	TSet<EUnlockType> UnlockedSet;


public:
	// Attrubutes begin
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	float GetHealthPercent() { return Attributes.GetHealthPercent(); }
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	float GetStaminaPercent() { return Attributes.GetStaminaPercent(); }
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	float GetManaPercent() { return Attributes.GetManaPercent(); }
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	float GetExperiencePercent() { return Attributes.GetExperiencePercent(); }

	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	void AddHealth(float Value) { Attributes.AddHealth(Value); }
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	void AddStamina(float Value) { Attributes.AddStamina(Value); }
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	void AddMana(float Value) { Attributes.AddMana(Value); }
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	bool AddExperience(float Value) { return Attributes.AddExperience(Value); }

	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	void AddMaxHealth(float Value) { Attributes.AddMaxHealth(Value); }
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	void AddMaxStamina(float Value) { Attributes.AddMaxStamina(Value); }
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	void AddMaxMana(float Value) { Attributes.AddMaxMana(Value); }

	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	void SubtractHealth(float Value) { Attributes.SubtractHealth(Value); }
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	void SubtractStamina(float Value) { Attributes.SubtractStamina(Value); }
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	void SubtractMana(float Value) { Attributes.SubtractMana(Value); }

	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	void AddBaseDamage(float Value) { Attributes.AddBaseDamage(Value); }

	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	float GetBaseDamage() { return Attributes.GetBaseDamage(); }

	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	int GetCharacterLevel() { return Attributes.GetCharacterLevel(); }

	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	int GetStrength() { return Attributes.GetStrength(); }
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	int GetAgility() { return Attributes.GetAgility(); }
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	int GetIntelligence() { return Attributes.GetIntelligence(); }
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	void AddStrength(int Value) { Attributes.Strength += Value; }
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	void AddAgility(int Value) { Attributes.Agility += Value; }
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	void AddIntelligence(int Value) { Attributes.Intelligence += Value; }


	// Attributes end

	// Unlocks begin
	UFUNCTION(BlueprintCallable, Category = "UnlockFunctions")
	bool HasUnlock(EUnlockType Type) { return UnlockedSet.Contains(Type); }
	UFUNCTION(BlueprintCallable, Category = "UnlockFunctions")
	void Unlock(EUnlockType Type) { UnlockedSet.Add(Type); }
	UFUNCTION(BlueprintCallable, Category = "UnlockFunctions")
	void Lock(EUnlockType Type) { UnlockedSet.Remove(Type); }
	// Unlocks end
	
};
