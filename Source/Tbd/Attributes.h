#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
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

	inline float GetBaseDamage() const { return BaseDamage; }

	inline void AddMaxHealth(float Value) { MaxHealth += Value; Health += Value; }
	inline void AddMaxStamina(float Value) { MaxStamina += Value; Stamina += Value; }
	inline void AddMaxMana(float Value) { MaxMana += Value; Mana += Value; }
	inline void AddHealth(float Value) { Health = FMath::Clamp(Health + Value, 0.f, MaxHealth); }
	inline void AddStamina(float Value) { Stamina = FMath::Clamp(Stamina + Value, 0.f, MaxStamina); }
	inline void AddMana(float Value) { Mana = FMath::Clamp(Mana + Value, 0.f, MaxMana); }

	inline void SubtractHealth(float Value) { Health = FMath::Clamp(Health - Value, 0.f, MaxHealth); }
	inline void SubtractStamina(float Value) { Stamina = FMath::Clamp(Stamina - Value, 0.f, MaxStamina); }
	inline void SubtractMana(float Value) { Mana = FMath::Clamp(Mana - Value, 0.f, MaxMana); }

	inline void AddBaseDamage(float Value) { BaseDamage += Value; }

	inline int GetCharacterLevel() const { return Level; }

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

	inline int GetStrength() const { return Strength; }
	inline int GetAgility() const { return Agility; }
	inline int GetIntelligence() const { return Intelligence; }
	inline void AddStrength(int Value) { Strength += Value; }
	inline void AddAgility(int Value) { Agility += Value; }
	inline void AddIntelligence(int Value) { Intelligence += Value; }


};


UENUM(BlueprintType)
enum class EUnlockType : uint8 {
	Q,
	Dodge,
	NightVision,
	Minimap
};

UENUM(BlueprintType)
enum class ECharacterType : uint8 {

	ECT_None UMETA(DisplayName = "None"),

	ECT_Knight = 1 << 0,

	ECT_Mage = 1 << 1,

	ECT_GameCharacter = ECT_Knight | ECT_Mage

};
ENUM_CLASS_FLAGS(ECharacterType)

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
	bool Reusability;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
	int ID;

	FUpgradeData() 
		: CharacterType(ECharacterType::ECT_None)
		, Name(NAME_None)
		, DisplayName(FText::GetEmpty())
		, Description(FText::GetEmpty())
		, Icon(nullptr)
		, Reusability(false)
		, ID(0)
	{
	}

	bool operator==(const FUpgradeData& Other) const
	{
		return ID == Other.ID;
	}
	
};

USTRUCT(BlueprintType)
struct FHoldUpgradeData {
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoldUpgradeData")
	FUpgradeData UpgradeData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoldUpgradeData")
	int Count;

	bool operator==(const FHoldUpgradeData& Other) const
	{
		return UpgradeData.ID == Other.UpgradeData.ID;
	}

};

USTRUCT(BlueprintType)
struct FUpgrades {
	GENERATED_BODY()
	TMap<FName, int> UpgradeCounts;
	TArray<FName> AvailableUpgrades;


	
	void InitUpgrades(UDataTable* DataTable, ECharacterType CharacterType) {
		if (!DataTable) return;
		UpgradeCounts.Empty();
		AvailableUpgrades.Empty();
		const TArray<FName> RowNames = DataTable->GetRowNames();
		for (const FName& RowName : RowNames)
		{
			FUpgradeData* Row = DataTable->FindRow<FUpgradeData>(RowName, TEXT(""));
			if (!Row) continue;
			if ((Row->CharacterType & CharacterType) != ECharacterType::ECT_None)
			{
				UpgradeCounts.Add(Row->Name, 0);
				AvailableUpgrades.Add(Row->Name);
			}
		}
	}

	void AddUpgrade(const FName& UpgradeName) {
		int* Count = UpgradeCounts.Find(UpgradeName);
		if (Count) {
			(*Count)++;
		}
		else {
			ensureMsgf(false, TEXT("Upgrade not found: %s"), *UpgradeName.ToString());
		}
	}

	bool HasUpgrade(const FName& UpgradeName) const {
		const int* Count = UpgradeCounts.Find(UpgradeName);
		return Count && *Count > 0;
	}

	void SetUpgrades(const TMap<FName, int>& NewCounts) {
		UpgradeCounts = NewCounts;
	}

	const TMap<FName, int>& GetUpgrades() const {
		return UpgradeCounts;
	}

	const int GetUpgradeCount(const FName& UpgradeName) const {
		const int* Count = UpgradeCounts.Find(UpgradeName);
		return Count ? *Count : 0;
	}

	const TArray<FName>& GetAvailableUpgrades() const {
		return AvailableUpgrades;
	}
	void SetAvailableUpgrades(const TArray<FName>& NewAvailableUpgrades) {
		AvailableUpgrades = NewAvailableUpgrades;
	}
	
};

USTRUCT(BlueprintType)
struct FSkillUI {

	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillUI")
	UTexture2D* Icon = nullptr;
	float CooldownDuration = 0;
	FGameplayTag CooldownTag;

};