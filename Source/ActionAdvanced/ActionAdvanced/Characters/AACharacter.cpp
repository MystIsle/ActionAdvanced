// Fill out your copyright notice in the Description page of Project Settings.


#include "AACharacter.h"
#include "ACCharacterMovementComponent.h"
#include "ACActionComponent.h"


AAACharacter::AAACharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UACCharacterMovementComponent>(CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;
	
	ActionComponent = CreateDefaultSubobject<UACActionComponent>(TEXT("ActionComponent"));
}
