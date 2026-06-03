// Fill out your copyright notice in the Description page of Project Settings.


#include "AACharacter.h"
#include "AACharacterMovementComponent.h"

AAACharacter::AAACharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UAACharacterMovementComponent>(
		ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AAACharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AAACharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AAACharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

