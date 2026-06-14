// Fill out your copyright notice in the Description page of Project Settings.


#include "AACharacter.h"
#include "ACCharacterMovementComponent.h"
#include "ACActionComponent.h"
#include "MeleeTraceComponent.h"
#include "ACHitReactionComponent.h"
#include "Components/ComboHandlerComponent.h"


AAACharacter::AAACharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UACCharacterMovementComponent>(CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;
	
	ActionComponent = CreateDefaultSubobject<UACActionComponent>(TEXT("ActionComponent"));
	MotionWarpingComponent = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarping"));
	MeleeTraceComponent = CreateDefaultSubobject<UMeleeTraceComponent>(TEXT("MeleeTrace"));
	HitReactionComponent = CreateDefaultSubobject<UACHitReactionComponent>(TEXT("HitReaction"));
	ComboHandlerComponent = CreateDefaultSubobject<UComboHandlerComponent>(TEXT("ComboHandler"));
}

void AAACharacter::BeginPlay()
{
	Super::BeginPlay();

	// 액션이 끝나 유휴로 돌아오면 콤보를 idle로 리셋(다음 입력은 새 Opener).
	if (ActionComponent && ComboHandlerComponent)
	{
		ActionComponent->OnReturnedToIdle.AddUObject(ComboHandlerComponent, &UComboHandlerComponent::NotifyComboActionEnded);
	}
}

void AAACharacter::ReceiveHit(const FACHitInfo& HitInfo)
{
	if (ActionComponent)
	{
		ActionComponent->StopActiveAction();
	}
	
	if (HitReactionComponent)
	{
		HitReactionComponent->PlayReact(HitInfo);
	}
}
