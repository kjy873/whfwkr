#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Attributes.generated.h"

UENUM(BlueprintType)
enum class EDamageType : uint8
{
	EDT_None UMETA(DisplayName = "None"),
};

USTRUCT(BlueprintType)
struct FAttribute
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute")
	float Health;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute")
	float MaxHealth;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute")
	float Stamina;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute")
	float MaxStamina;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute")
	float Mana;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute")
	float MaxMana;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute")
	float Experience;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute")
	float MaxExperience;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute")
	int Level;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute")
	int Strength;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute")
	int Agility;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute")
	int Intelligence;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute")
	float BaseDamage;

	FAttribute()
		: Health(100.f)
		, MaxHealth(100.f)
		, Stamina(100.f)
		, MaxStamina(100.f)
		, Mana(100.f)
		, MaxMana(100.f)
		, Experience(0.f)
		, MaxExperience(100.f)
		, Level(1)
		, Strength(1)
		, Agility(1)
		, Intelligence(1)
		, BaseDamage(10.f)
		
	{
	}

	inline float GetHealthPercent() const { return MaxHealth > 0.f ? Health / MaxHealth : 0.f; }
	inline float GetStaminaPercent() const { return MaxStamina > 0.f ? Stamina / MaxStamina : 0.f; }
	inline float GetManaPercent() const { return MaxMana > 0.f ? Mana / MaxMana : 0.f; }
	inline float GetExperiencePercent() const { return MaxExperience > 0.f ? Experience / MaxExperience : 0.f; }

	inline void AddMaxHealth(float Value) { MaxHealth += Value; Health += Value; }
	inline void AddMaxStamina(float Value) { MaxStamina += Value; Stamina += Value; }
	inline void AddMaxMana(float Value) { MaxMana += Value; Mana += Value; }
	inline void AddHealth(float Value) { Health = FMath::Clamp(Health + Value, 0.f, MaxHealth); }
	inline void AddStamina(float Value) { Stamina = FMath::Clamp(Stamina + Value, 0.f, MaxStamina); }
	inline void AddMana(float Value) { Mana = FMath::Clamp(Mana + Value, 0.f, MaxMana); }

	inline void SubtractHealth(float Value) { Health = FMath::Clamp(Health - Value, 0.f, MaxHealth); }
	inline void SubtractStamina(float Value) { Stamina = FMath::Clamp(Stamina - Value, 0.f, MaxStamina); }
	inline void SubtractMana(float Value) { Mana = FMath::Clamp(Mana - Value, 0.f, MaxMana); }

	inline bool AddExperience(float Value)
	{
		bool LevelUp = false;

		Experience += Value;
		while (Experience >= MaxExperience) {
			Experience -= MaxExperience;
			Level++;
			MaxExperience *= 1.1f;
			LevelUp = true;
		}

		return LevelUp;
	}


};

UENUM(BlueprintType)
enum class ECharacterType : uint8 {

	ECT_None UMETA(DisplayName = "None"),

	ECT_Knight = 1 << 0,

	ECT_Mage = 1 << 1,

	ECT_GameCharacter = ECT_Knight | ECT_Mage

};

USTRUCT(BlueprintType)
struct FUpgradeData : public FTableRowBase{

	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
	ECharacterType CharacterType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
	FName Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
	UTexture2D* Icon;

	FUpgradeData() 
		: CharacterType(ECharacterType::ECT_None)
		, Name(NAME_None)
		, DisplayName(FText::GetEmpty())
		, Description(FText::GetEmpty())
		, Icon(nullptr)
	{
	}

};