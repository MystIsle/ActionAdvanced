// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ACHitEffect.generated.h"

class UCurveFloat;

USTRUCT(BlueprintType)
struct ACTIONCORE_API FACHitEffect
{
	GENERATED_BODY()

	// 히트스탑(타격 순간 멈칫). 0이면 없음.
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0.0"))
	float HitStopDuration = 0.f;

	// 히트스탑 중 GlobalAnimRateScale. 0 금지(거의 정지).
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float HitStopPlayRate = 0.05f;

	// 넉백 거리(cm). 0이면 넉백 없음.
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0.0"))
	float KnockbackDistance = 0.f;

	// 넉백 지속 시간(s).
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0.0"))
	float KnockbackDuration = 0.1f;

	// 넉백 이동 이징(X=정규화 시간 0~1, Y=진행도 0~1). null이면 등속.
	UPROPERTY(EditAnywhere)
	TObjectPtr<UCurveFloat> KnockbackEaseCurve;
};
