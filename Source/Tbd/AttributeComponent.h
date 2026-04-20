// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Attributes.h"
#include "Components/ActorComponent.h"
#include "AttributeComponent.generated.h"


UCLASS( Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TBD_API UAttributeComponent : public UActorComponent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute", meta = (AllowPrivateAccess = "true"))
	FAttribute Attributes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unlock", meta = (AllowPrivateAccess = "true"))
	FUpgrades Upgrades;

public:	
	// Sets default values for this component's properties
	UAttributeComponent();

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
	int GetStrength() const { return Attributes.GetStrength(); }
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	int GetAgility() const { return Attributes.GetAgility(); }
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	int GetIntelligence() const { return Attributes.GetIntelligence(); }

	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	void AddStrength(int Value) { Attributes.AddStrength(Value); }
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	void AddAgility(int Value) { Attributes.AddAgility(Value); }
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	void AddIntelligence(int Value) { Attributes.AddIntelligence(Value); }

	UFUNCTION(BlueprintCallable, Category = "PlayerData")
	FAttribute GetAttributes() const { return Attributes; }
	UFUNCTION(BlueprintCallable, Category = "PlayerData")
	void SetAttributes(const FAttribute& NewAttributes) { Attributes = NewAttributes; }

	// Attributes end

	// Upgrades begin

	UFUNCTION(BlueprintCallable, Category = "PlayerData")
	const TMap<FName, int>& GetUpgrades() const { return Upgrades.UpgradeCounts; }
	UFUNCTION(BlueprintCallable, Category = "PlayerData")
	void SetUpgrades(const TMap<FName, int>& NewUpgrades) { Upgrades.UpgradeCounts = NewUpgrades; }
	UFUNCTION(BlueprintCallable, Category = "PlayerData")
	void AddUpgrade(const FName& UpgradeName) { Upgrades.AddUpgrade(UpgradeName); }
	UFUNCTION(BlueprintCallable, Category = "PlayerData")
	bool HasUpgrade(const FName& UpgradeName) const { return Upgrades.HasUpgrade(UpgradeName); }
	UFUNCTION(BlueprintCallable, Category = "PlayerData")
	int GetUpgradeCount(const FName& UpgradeName) const { return Upgrades.GetUpgradeCount(UpgradeName); }
	UFUNCTION(BlueprintCallable, Category = "PlayerData")
	TArray<FName> GetAvailableUpgrades() const { return Upgrades.GetAvailableUpgrades(); }
	UFUNCTION(BlueprintCallable, Category = "PlayerData")
	void SetAvailableUpgrades(const TArray<FName>& NewAvailableUpgrades) { Upgrades.SetAvailableUpgrades(NewAvailableUpgrades); }

	// Upgrades end

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};
