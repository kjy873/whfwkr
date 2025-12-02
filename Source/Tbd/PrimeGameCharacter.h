// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Attributes.h"
#include "GameFramework/Character.h"
#include "PrimeGameCharacter.generated.h"

UCLASS()
class TBD_API APrimeGameCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	APrimeGameCharacter();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CharacterAttribute")
	float MoveDirectionAngle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CharacterAttribute")
	int MaxEXP;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CharacterAttribute")
	int CurrentEXP;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CharacterAttribute")
	int CurrentLevel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CharacterAttribute")
	bool Aiming;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CharacterAttribute")
	ECharacterType CharacterType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CharacterAttribute")
	FAttribute Attributes;


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	float GetHealth() const { return Attributes.Health; }
	float GetMaxHealth() const { return Attributes.MaxHealth; }
	float GetStamina() const { return Attributes.Stamina; }
	float GetMaxStamina() const { return Attributes.MaxStamina; }
	float GetMana() const { return Attributes.Mana; }
	float GetMaxMana() const { return Attributes.MaxMana; }
	float GetBaseDamage() const { return Attributes.BaseDamage; }
	float GetStrength() const { return Attributes.Strength; }
	float GetAgility() const { return Attributes.Agility; }
	float GetIntelligence() const { return Attributes.Intelligence; }

	float AddHealth(float Value) {
		Attributes.Health = FMath::Clamp(Attributes.Health + Value, 0.f, Attributes.MaxHealth);
		return Attributes.Health;
	}

	float SubtractHealth(float Value) {
		Attributes.Health = FMath::Clamp(Attributes.Health - Value, 0.f, Attributes.MaxHealth);
		return Attributes.Health;
	}

	float AddStamina(float Value) {
		Attributes.Stamina = FMath::Clamp(Attributes.Stamina + Value, 0.f, Attributes.MaxStamina);
		return Attributes.Stamina;
	}

	float SubtractStamina(float Value) {
		Attributes.Stamina = FMath::Clamp(Attributes.Stamina - Value, 0.f, Attributes.MaxStamina);
		return Attributes.Stamina;
	}

	float AddMana(float Value) {
		Attributes.Mana = FMath::Clamp(Attributes.Mana + Value, 0.f, Attributes.MaxMana);
		return Attributes.Mana;
	}

	float SubtractMana(float Value) {
		Attributes.Mana = FMath::Clamp(Attributes.Mana - Value, 0.f, Attributes.MaxMana);
		return Attributes.Mana;
	}

	float SetMaxHealth(float Value) {
		Attributes.MaxHealth = Value;
		return Attributes.MaxHealth;
	}

	float SetMaxStamina(float Value) {
		Attributes.MaxStamina = Value;
		return Attributes.MaxStamina;
	}

	float SetMaxMana(float Value) {
		Attributes.MaxMana = Value;
		return Attributes.MaxMana;
	}

	float SetBaseDamage(float Value) {
		Attributes.BaseDamage = Value;
		return Attributes.BaseDamage;
	}

};
