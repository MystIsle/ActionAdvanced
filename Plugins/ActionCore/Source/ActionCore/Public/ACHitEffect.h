// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "ACHitEffect.generated.h"

class UCurveFloat;
class UNiagaraSystem;
class UCameraShakeBase;

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

	// 피격 메시 지터 진폭(cm). 0이면 셰이크 없음. 지속은 HitStopDuration에 동기화.
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0.0"))
	float MeshShakeAmplitude = 0.f;

	// 피격 메시 지터 스텝 수(히트스톱 동안 흔들 횟수). 적을수록 뚝뚝, 많을수록 버즈.
	UPROPERTY(EditAnywhere, meta = (ClampMin = "1"))
	int32 MeshShakeSampleCount = 4;

	// 넉백 거리(cm). 0이면 넉백 없음.
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0.0"))
	float KnockbackDistance = 0.f;

	// 넉백 지속 시간(s).
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0.0"))
	float KnockbackDuration = 0.1f;

	// 넉백 이동 이징(X=정규화 시간 0~1, Y=진행도 0~1). null이면 등속.
	UPROPERTY(EditAnywhere)
	TObjectPtr<UCurveFloat> KnockbackEaseCurve;

	// 피격 임팩트 VFX. 히트 위치에 비부착 스폰, 때린 쪽(-넉백방향)을 향함. null이면 없음.
	UPROPERTY(EditAnywhere)
	TObjectPtr<UNiagaraSystem> HitNiagara;

	// 히트 FX 스폰 스케일. 에셋 모듈이 오너 스케일을 안 받으면 안 먹을 수 있음.
	UPROPERTY(EditAnywhere)
	FVector HitFXScale = FVector(1.0);

	UPROPERTY(EditAnywhere, meta = (InlineEditConditionToggle))
	bool bHitFXPreSim = false;

	// 히트 FX 프리롤(s): 스폰 직후 시뮬레이션을 미리 돌려 피크 프레임에서 동결되게.
	UPROPERTY(EditAnywhere, meta = (EditCondition = "bHitFXPreSim", ClampMin = "0.0", ClampMax = "1.0"))
	float HitFXPreSimTime = 0.1f;

	UPROPERTY(EditAnywhere, meta = (InlineEditConditionToggle))
	bool bHitFXMinTimeScale = false;

	// 히트스탑 동안 FX 타임스케일 하한(슬로모). 끄면 완전 동결.
	UPROPERTY(EditAnywhere, meta = (EditCondition = "bHitFXMinTimeScale", ClampMin = "0.0", ClampMax = "1.0"))
	float HitFXMinTimeScale = 0.2f;

	// 공격자 히트 확정 카메라 셰이크. null이면 없음. 때린 쪽이 플레이어일 때만 발동(AI 자동 무시).
	UPROPERTY(EditAnywhere)
	TSubclassOf<UCameraShakeBase> HitCameraShake;

	// 카메라 셰이크 강도 배수(ClientStartCameraShake Scale). 방향은 코드가 공격 방향으로 정렬.
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0.0"))
	float HitCameraShakeScale = 1.f;
};

// 한 번의 피격 묶음. OnMeleeTraceHit → IACHittable::ReceiveHit로 전달.
USTRUCT(BlueprintType)
struct ACTIONCORE_API FACHitInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FACHitEffect Effect;

	UPROPERTY(BlueprintReadWrite)
	FVector Direction = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite)
	FVector HitLocation = FVector::ZeroVector;
};
