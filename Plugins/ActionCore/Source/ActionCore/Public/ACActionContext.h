// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

struct FBranchingPointNotifyPayload;
class ACharacter;
class UACActionInstance;

struct ACTIONCORE_API FACActionContext
{
	explicit FACActionContext(const FBranchingPointNotifyPayload& Payload);

	bool IsValid() const { return Owner.IsValid() && Instance.IsValid(); }

	TWeakObjectPtr<ACharacter>        Owner;
	TWeakObjectPtr<UACActionInstance> Instance;
};
