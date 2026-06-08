// Fill out your copyright notice in the Description page of Project Settings.

#include "ACAnimNotifyState_MeleeHit.h"

#include "ACActionContext.h"
#include "ACActionInstance.h"

UACAnimNotifyState_MeleeHit::UACAnimNotifyState_MeleeHit()
{
	bIsNativeBranchingPoint = true;
}

void UACAnimNotifyState_MeleeHit::BranchingPointNotifyBegin(FBranchingPointNotifyPayload& BranchingPointPayload)
{
	// Super가 부모 MeleeTrace의 NotifyBegin을 호출 → trace 시작.
	Super::BranchingPointNotifyBegin(BranchingPointPayload);

	const FACActionContext Context(BranchingPointPayload);
	if (Context.IsValid())
	{
		Context.Instance->BeginHitDetection(HitEffect);
	}
}

void UACAnimNotifyState_MeleeHit::BranchingPointNotifyEnd(FBranchingPointNotifyPayload& BranchingPointPayload)
{
	// Super가 부모 MeleeTrace의 NotifyEnd를 호출 → trace 종료.
	Super::BranchingPointNotifyEnd(BranchingPointPayload);

	const FACActionContext Context(BranchingPointPayload);
	if (Context.IsValid())
	{
		Context.Instance->EndHitDetection();
	}
}
