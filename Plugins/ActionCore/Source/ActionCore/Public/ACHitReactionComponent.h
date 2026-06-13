// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ACHitEffect.h"
#include "ACHitReactionComponent.generated.h"

class ACharacter;
class UACCharacterMovementComponent;
class UACMontageInstanceController;
class UAnimMontage;
class UCurveFloat;
class UMaterialInstanceDynamic;

UCLASS(ClassGroup = (ActionCore), meta = (BlueprintSpawnableComponent))
class ACTIONCORE_API UACHitReactionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UACHitReactionComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void PlayReact(const FACHitEffect& Effect, const FVector& Direction, const FVector& HitLocation);
	void RequestHitStop(float Scale, float Duration, int32 MontageInstanceID = INDEX_NONE);

private:
	// 리액션 시퀀스
	void FaceAttacker(const FVector& HitDirection);
	int32 PlayHitReactMontage();
	void SpawnHitFX(const FACHitEffect& Effect, const FVector& Direction, const FVector& HitLocation, int32 MontageInstanceID);
	void SetBrainLogicPaused(bool bPaused);
	void StartKnockback();
	void FinishReact();
	void CancelReact();

	// 히트스탑
	void OnHitStopFinished();
	void RestoreFXTimeScale();

	// 히트 점멸
	void StartHitFlash();
	void EndHitFlash();

	// 유틸
	UACMontageInstanceController* FindMontageInstanceController(int32 MontageInstanceID) const;

	// 캐시된 컴포넌트
	UPROPERTY(Transient)
	TObjectPtr<ACharacter> OwnerCharacter;

	UPROPERTY(Transient)
	TObjectPtr<UACCharacterMovementComponent> MovementComponent;

	// 설정
	UPROPERTY(EditAnywhere, Category = "HitReaction")
	TArray<TObjectPtr<UAnimMontage>> HitReactMontages;

	// 리액션 상태
	FTimerHandle ReactTimer;
	int32 HitMontageIndex = 0;
	bool bReacting = false;

	// 넉백
	FVector PendingKnockbackDir = FVector::ZeroVector;
	float PendingKnockbackDistance = 0.f;
	float PendingKnockbackDuration = 0.f;

	UPROPERTY(Transient)
	TObjectPtr<UCurveFloat> PendingKnockbackCurve;

	uint16 KnockbackSourceID = 0;

	// 히트스탑
	FTimerHandle HitStopTimer;

	// 현재 히트스탑이 FX 스케일을 걸어둔 컨트롤러(복원 대상).
	TWeakObjectPtr<UACMontageInstanceController> ScaledFXController;

	// 히트 점멸(전역 UACCombatFeelSettings 구동). falloff는 머티리얼 Time 기반 자체 계산.
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> FlashMID;

	FTimerHandle FlashTimer;
};
