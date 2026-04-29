// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Attributes.h"
#include "Components/ActorComponent.h"
#include "AttributeComponent.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthChanged, float, Rate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStaminaChanged, float, Rate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnManaChanged, float, Rate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEXPChanged, float, Rate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLevelChanged, int, Rate);

struct FAttribute;

UCLASS( Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TBD_API UAttributeComponent : public UActorComponent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attributes", meta = (AllowPrivateAccess = "true"))
	FAttribute Attributes;
	
	// Delegates
	UPROPERTY(BlueprintAssignable, Category = "AttributeDelegates")
	FOnHealthChanged OnHealthChanged;
	UPROPERTY(BlueprintAssignable, Category = "AttributeDelegates")
	FOnStaminaChanged OnStaminaChanged;
	UPROPERTY(BlueprintAssignable, Category = "AttributeDelegates")
	FOnManaChanged OnManaChanged;
	UPROPERTY(BlueprintAssignable, Category = "AttributeDelegates")
	FOnEXPChanged OnEXPChanged;
	UPROPERTY(BlueprintAssignable, Category = "AttributeDelegates")
	FOnLevelChanged OnLevelChanged;



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
	void AddHealth(float Value) { Attributes.AddHealth(Value); OnHealthChanged.Broadcast(Attributes.GetHealthPercent()); }
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	void AddStamina(float Value) { Attributes.AddStamina(Value); OnStaminaChanged.Broadcast(Attributes.GetStaminaPercent()); }
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	void AddMana(float Value) { Attributes.AddMana(Value); OnManaChanged.Broadcast(Attributes.GetManaPercent()); }
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	bool AddExperience(float Value) { 
		bool LevelUp = Attributes.AddExperience(Value); 
		OnLevelChanged.Broadcast(Attributes.GetCharacterLevel());
		return LevelUp;
	}

	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	void AddMaxHealth(float Value) { Attributes.AddMaxHealth(Value); OnHealthChanged.Broadcast(Attributes.GetHealthPercent()); }
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	void AddMaxStamina(float Value) { Attributes.AddMaxStamina(Value); OnStaminaChanged.Broadcast(Attributes.GetStaminaPercent()); }
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	void AddMaxMana(float Value) { Attributes.AddMaxMana(Value); OnManaChanged.Broadcast(Attributes.GetManaPercent()); }

	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	void SubtractHealth(float Value) { Attributes.SubtractHealth(Value); OnHealthChanged.Broadcast(Attributes.GetHealthPercent()); }
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	void SubtractStamina(float Value) { Attributes.SubtractStamina(Value); OnStaminaChanged.Broadcast(Attributes.GetStaminaPercent()); }
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	void SubtractMana(float Value) { Attributes.SubtractMana(Value); OnManaChanged.Broadcast(Attributes.GetManaPercent()); }

	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	void AddBaseDamage(float Value) { Attributes.AddBaseDamage(Value); }

	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	float GetBaseDamage() const { return Attributes.GetBaseDamage(); }

	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	int GetCharacterLevel() const { return Attributes.GetCharacterLevel(); }

	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	int GetStrength() const { return Attributes.GetStrength(); }
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	int GetAgility() const { return Attributes.GetAgility(); }
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	int GetIntelligence() const { return Attributes.GetIntelligence(); }
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	void AddStrength(int Value);
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	void AddAgility(int Value);
	UFUNCTION(BlueprintCallable, Category = "AttributeFunctions")
	void AddIntelligence(int Value);
protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};
