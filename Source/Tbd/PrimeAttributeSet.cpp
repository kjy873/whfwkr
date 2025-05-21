// Fill out your copyright notice in the Description page of Project Settings.


#include "PrimeAttributeSet.h"

UPrimeAttributeSet::UPrimeAttributeSet() :
	Health(100.f), MaxHealth(100.f), Mana(100.f), MaxMana(100.f), Defense(4.f), Strength(3.f), Stamina(100.f), MaxStamina(100.f), MoveSpeed(600.f)
{

}

void UPrimeAttributeSet::OnRep_Health(const FGameplayAttributeData& OldCurrentHealth) {
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPrimeAttributeSet, Health, OldCurrentHealth);
}

void UPrimeAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPrimeAttributeSet, MaxHealth, OldMaxHealth);
}

void UPrimeAttributeSet::OnRep_Mana(const FGameplayAttributeData& OldMana)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPrimeAttributeSet, Mana, OldMana);
}

void UPrimeAttributeSet::OnRep_MaxMana(const FGameplayAttributeData& OldMaxMana)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPrimeAttributeSet, MaxMana, OldMaxMana);
}

void UPrimeAttributeSet::OnRep_Defense(const FGameplayAttributeData& OldDefense)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPrimeAttributeSet, Defense, OldDefense);
}

void UPrimeAttributeSet::OnRep_MoveSpeed(const FGameplayAttributeData& OldMoveSpeed)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPrimeAttributeSet, MoveSpeed, OldMoveSpeed);
}

void UPrimeAttributeSet::OnRep_Strength(const FGameplayAttributeData& OldStrength)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPrimeAttributeSet, Strength, OldStrength);
}

void UPrimeAttributeSet::OnRep_Stamina(const FGameplayAttributeData& OldStamina)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPrimeAttributeSet, Stamina, OldStamina);
}

void UPrimeAttributeSet::OnRep_MaxStamina(const FGameplayAttributeData& OldMaxStamina)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPrimeAttributeSet, MaxStamina, OldMaxStamina);
}

void UPrimeAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {

	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UPrimeAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPrimeAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPrimeAttributeSet, Mana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPrimeAttributeSet, MaxMana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPrimeAttributeSet, Stamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPrimeAttributeSet, MaxStamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPrimeAttributeSet, Defense, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPrimeAttributeSet, Strength, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPrimeAttributeSet, MoveSpeed, COND_None, REPNOTIFY_Always);

}