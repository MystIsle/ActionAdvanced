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
	void StartKnockback();
	void FinishReact();
	void CancelReact();
	void OnHitStopFinished();
	void RestoreFXTimeScale();
	UACMontageInstanceController* FindMontageInstanceController(int32 MontageInstanceID) const;

	UPROPERTY(EditAnywhere, Category = "HitReaction")
	TArray<TObjectPtr<UAnimMontage>> HitReactMontages;

	UPROPERTY(Transient)
	TObjectPtr<ACharacter> OwnerCharacter;

	UPROPERTY(Transient)
	TObjectPtr<UACCharacterMovementComponent> MovementComponent;

	FTimerHandle ReactTimer;
	FTimerHandle HitStopTimer;

	FVector PendingKnockbackDir = FVector::ZeroVector;
	float PendingKnockbackDistance = 0.f;
	float PendingKnockbackDuration = 0.f;

	UPROPERTY(Transient)
	TObjectPtr<UCurveFloat> PendingKnockbackCurve;

	// 현재 히트스탑이 FX 스케일을 걸어둔 컨트롤러(복원 대상).
	TWeakObjectPtr<UACMontageInstanceController> ScaledFXController;
	uint16 KnockbackSourceID = 0;
	int32 HitMontageIndex = 0;
	bool bReacting = false;
};
