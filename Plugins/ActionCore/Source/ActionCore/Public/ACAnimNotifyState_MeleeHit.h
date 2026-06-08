// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AnimNotifyState_MeleeTrace.h"
#include "ACHitEffect.h"
#include "ACAnimNotifyState_MeleeHit.generated.h"

UCLASS(DisplayName = "AC Melee Hit")
class ACTIONCORE_API UACAnimNotifyState_MeleeHit : public UAnimNotifyState_MeleeTrace
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "HitReaction")
	FACHitEffect HitEffect;

	UACAnimNotifyState_MeleeHit();

	virtual void BranchingPointNotifyBegin(FBranchingPointNotifyPayload& BranchingPointPayload) override;
	virtual void BranchingPointNotifyEnd(FBranchingPointNotifyPayload& BranchingPointPayload) override;
};
