// Fill out your copyright notice in the Description page of Project Settings.


#include "AttributeComponent.h"


// Sets default values for this component's properties
UAttributeComponent::UAttributeComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UAttributeComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UAttributeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UAttributeComponent::AddStrength(int Value)
{
	Attributes.AddStrength(Value);
	Attributes.AddMaxHealth(Value * 30.0f);
	Attributes.AddBaseDamage(Value * 1.0f);
	OnStrengthChanged.Broadcast(Attributes.GetStrength());
}

void UAttributeComponent::AddAgility(int Value)
{
	Attributes.AddAgility(Value);
	Attributes.AddMaxStamina(Value * 30.0f);
	Attributes.AddBaseDamage(Value * 1.5f);
	OnAgilityChanged.Broadcast(Attributes.GetAgility());
}

void UAttributeComponent::AddIntelligence(int Value)
{
	Attributes.AddIntelligence(Value);
	Attributes.AddMaxMana(Value * 30.0f);
	Attributes.AddBaseDamage(Value * 3.0f);
	OnIntelligenceChanged.Broadcast(Attributes.GetIntelligence());
}