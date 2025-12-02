// Fill out your copyright notice in the Description page of Project Settings.


#include "PrimeGameCharacter.h"

// Sets default values
APrimeGameCharacter::APrimeGameCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void APrimeGameCharacter::BeginPlay()
{
	Super::BeginPlay();


}

// Called every frame
void APrimeGameCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void APrimeGameCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

