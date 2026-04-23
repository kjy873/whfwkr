// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Attributes.h"
#include "Components/ActorComponent.h"
#include "AttributeComponent.generated.h"


struct FAttribute;

UCLASS( Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TBD_API UAttributeComponent : public UActorComponent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attributes", meta = (AllowPrivateAccess = "true"))
	FAttribute Attributes;
	


public:	
	// Sets default values for this component's properties
	UAttributeComponent();

	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	float GetHealthPercent() const { return Attributes.GetHealthPercent(); }
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	float GetStaminaPercent() const { return Attributes.GetStaminaPercent(); }
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	float GetManaPercent() const { return Attributes.GetManaPercent(); }
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	float GetExperiencePercent() const { return Attributes.GetExperiencePercent(); }

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
	int GetCharacterLevel() const { return Attributes.GetCharacterLevel(); }

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
protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};
