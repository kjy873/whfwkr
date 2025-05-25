// Fill out your copyright notice in the Description page of Project Settings.


#include "MeleeDamageCalculation.h"
#include "AbilitySystemComponent.h"
#include "PrimeAttributeSet.h"

struct MeleeDamageStatics {
	DECLARE_ATTRIBUTE_CAPTUREDEF(AttackDamage);
	DECLARE_ATTRIBUTE_CAPTUREDEF(Defense);
	DECLARE_ATTRIBUTE_CAPTUREDEF(Health);

	MeleeDamageStatics()
	{
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPrimeAttributeSet, AttackDamage, Source, true);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPrimeAttributeSet, Defense, Target, true);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPrimeAttributeSet, Health, Target, false);
	}
};

static const MeleeDamageStatics& GetMeleeDamageStatics()
{
	static MeleeDamageStatics DmgStatics;
	return DmgStatics;
}
UMeleeDamageCalculation::UMeleeDamageCalculation()
{
	RelevantAttributesToCapture.Add(GetMeleeDamageStatics().AttackDamageDef);
	RelevantAttributesToCapture.Add(GetMeleeDamageStatics().DefenseDef);
}

void UMeleeDamageCalculation::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	OUT FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();
	UAbilitySystemComponent* SourceASC = ExecutionParams.GetSourceAbilitySystemComponent();

	AActor* SourceActor = SourceASC ? SourceASC->GetAvatarActor_Direct() : nullptr;
	AActor* TargetActor = TargetASC ? TargetASC->GetAvatarActor_Direct() : nullptr;

	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluationParams;
	EvaluationParams.SourceTags = SourceTags;
	EvaluationParams.TargetTags = TargetTags;

	float AttackDamage = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(GetMeleeDamageStatics().AttackDamageDef, EvaluationParams, AttackDamage);

	float WeaponDamage = GetWeaponDamage(SourceActor);

	float Defense = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(GetMeleeDamageStatics().DefenseDef, EvaluationParams, Defense);

	float BlockAbsorption = GetBlockAbsorption(TargetActor);

	float DamageDone = GetDamageMagnitude(SourceActor) * (AttackDamage + WeaponDamage) - Defense - BlockAbsorption;

	if (DamageDone > 0) {
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(GetMeleeDamageStatics().HealthProperty, EGameplayModOp::Additive, -DamageDone));
		const_cast<UMeleeDamageCalculation*>(this)->OnDamageDone(SourceActor, TargetActor, DamageDone);
	}
	else {
		const_cast<UMeleeDamageCalculation*>(this)->OnFullDamageAbsorbed(SourceActor, TargetActor);
	}
}

float UMeleeDamageCalculation::GetDamageMagnitude_Implementation(AActor* SourceActor) const
{
	return 1.f;
}

float UMeleeDamageCalculation::GetWeaponDamage_Implementation(AActor* SourceActor) const
{

	return 0.f;
}

float UMeleeDamageCalculation::GetBlockAbsorption_Implementation(AActor* TargetActor) const
{

	return 0.f;
}

void UMeleeDamageCalculation::OnFullDamageAbsorbed_Implementation(AActor* SourceActor, AActor* TargetActor) const
{
	UE_LOG(LogTemp, Log, TEXT("Full damage absorbed from %s to %s"), *SourceActor->GetName(), *TargetActor->GetName());
}

void UMeleeDamageCalculation::OnDamageDone_Implementation(AActor* SourceActor, AActor* TargetActor, float Damage) const
{
	UE_LOG(LogTemp, Log, TEXT("Damage done from %s to %s: %.2f"), *SourceActor->GetName(), *TargetActor->GetName(), Damage);
}