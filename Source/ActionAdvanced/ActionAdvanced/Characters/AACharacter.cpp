// Fill out your copyright notice in the Description page of Project Settings.


#include "AACharacter.h"
#include "ACCharacterMovementComponent.h"
#include "ACActionComponent.h"
#include "MeleeTraceComponent.h"
#include "ACHitReactionComponent.h"


AAACharacter::AAACharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UACCharacterMovementComponent>(CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;
	
	ActionComponent = CreateDefaultSubobject<UACActionComponent>(TEXT("ActionComponent"));
	MotionWarpingComponent = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarping"));
	MeleeTraceComponent = CreateDefaultSubobject<UMeleeTraceComponent>(TEXT("MeleeTrace"));
	HitReactionComponent = CreateDefaultSubobject<UACHitReactionComponent>(TEXT("HitReaction"));
}

void AAACharacter::ReceiveHit(const FACHitInfo& HitInfo)
{
	if (ActionComponent)
	{
		ActionComponent->StopActiveAction();
	}
	if (HitReactionComponent)
	{
		HitReactionComponent->PlayReact(HitInfo.Effect, HitInfo.Direction, HitInfo.HitLocation);
	}
}
