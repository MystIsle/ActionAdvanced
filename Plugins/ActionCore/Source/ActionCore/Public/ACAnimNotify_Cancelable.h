// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "ACAnimNotify_Cancelable.generated.h"

UCLASS()
class ACTIONCORE_API UACAnimNotify_MoveCancelable : public UAnimNotify
{
	GENERATED_BODY()
	
public:
	UACAnimNotify_MoveCancelable();

	virtual FString GetNotifyName_Implementation() const override;
	virtual void BranchingPointNotify(FBranchingPointNotifyPayload& BranchingPointPayload) override;
};
