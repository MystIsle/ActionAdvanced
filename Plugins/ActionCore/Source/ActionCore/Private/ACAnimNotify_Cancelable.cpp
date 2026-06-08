// Fill out your copyright notice in the Description page of Project Settings.


#include "ACAnimNotify_Cancelable.h"

#include "ACActionContext.h"
#include "ACActionInstance.h"

UACAnimNotify_Cancelable::UACAnimNotify_Cancelable()
{
	bIsNativeBranchingPoint = true;
}

FString UACAnimNotify_Cancelable::GetNotifyName_Implementation() const
{
	return TEXT("액션 캔슬 가능");
}

void UACAnimNotify_Cancelable::BranchingPointNotify(FBranchingPointNotifyPayload& BranchingPointPayload)
{
	Super::BranchingPointNotify(BranchingPointPayload);

	const FACActionContext Context(BranchingPointPayload);
	if (Context.IsValid() == false)
	{
		return;
	}

	Context.Instance->MarkCancelable();
}
