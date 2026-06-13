// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ACHitEffect.h"
#include "ACHittable.generated.h"

UINTERFACE(MinimalAPI)
class UACHittable : public UInterface
{
	GENERATED_BODY()
};

// 피격을 받는 액터. OnMeleeTraceHit이 이 진입점 하나로 위임한다(액터가 각 컴포넌트에 분배).
class ACTIONCORE_API IACHittable
{
	GENERATED_BODY()

public:
	virtual void ReceiveHit(const FACHitInfo& HitInfo) = 0;
};
