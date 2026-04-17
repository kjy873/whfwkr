// Fill out your copyright notice in the Description page of Project Settings.


#include "MainPlayerState.h"

AMainPlayerState::AMainPlayerState()
{


}

void AMainPlayerState::Initialize() {
	Attributes.MaxHealth = 100.f;
	Attributes.Health = 100.f;
	Attributes.MaxStamina = 100.f;
	Attributes.Stamina = 100.f;
	Attributes.MaxMana = 100.f;
	Attributes.Mana = 100.f;
	Attributes.Experience = 0.f;
	Attributes.Level = 1;
	Attributes.BaseDamage = 10.f;
	Attributes.Agility = 1;
	Attributes.Strength = 1;
	Attributes.Intelligence = 1;

	Initialized = true;

	
}