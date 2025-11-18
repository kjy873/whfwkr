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
	int Strength;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute")
	int Agility;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute")
	int Intelligence;

	FAttribute()
		: Health(100.f)
		, MaxHealth(100.f)
		, Stamina(100.f)
		, MaxStamina(100.f)
		, Mana(100.f)
		, MaxMana(100.f)
		, Strength(1)
		, Agility(1)
		, Intelligence(1)
	{
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

};