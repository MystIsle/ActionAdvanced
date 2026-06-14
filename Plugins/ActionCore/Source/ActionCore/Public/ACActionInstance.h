// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MeleeTraceComponent.h"
#include "ACHitEffect.h"
#include "ACActionInstance.generated.h"

class UACCharacterMovementComponent;
class UACMontageInstanceController;
class UACActionDataAsset;
class UACActionComponent;
class UACAction;
class UAnimMontage;
class ACharacter;
class UACAnimInstance;

UENUM()
enum class EACActionInstanceState : uint8
{
	None,
	Ready,
	Playing,
	BlendingOut,
	Finished,
};

UCLASS()
class ACTIONCORE_API UACActionInstance : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(UACAction* InAction);
	bool Play(const FRotator& InRotation);
	void Stop();

	void BeginHitDetection(const FACHitEffect& InHitEffect);
	void EndHitDetection();

	EACActionInstanceState GetState() const { return State; }
	int32 GetMontageInstanceID() const { return MontageInstanceID; }
	bool IsPlaying() const { return State == EACActionInstanceState::Playing || State == EACActionInstanceState::BlendingOut; }
	bool IsActionCancelable() const;
	void MarkMoveCancelable();
	void SetActionCancelable(bool bInCancelable);

	// 이 재생이 오토타게팅 러시(타겟 확보)로 발동됐는지. 모션워핑 위치 워프 토글의 기준.
	bool IsLunging() const { return bLunging; }

private:
	void SetState(EACActionInstanceState NewState);
	void StopInternal(bool bNeedAnimStop);
	void SetMovementLocked(bool bLock);
	UACMontageInstanceController* GetMontageInstanceController() const;
	FRotator DetermineFacingRotation(const FRotator& InputRotation) const;

	// 오토타게팅 러시 워프 목표(위치+회전)를 계산한다. 타겟이 없거나 거리/각도 밖이면 false(회전 폴백).
	bool TryLungeWarp(const FRotator& InRotation, FVector& OutLocation, FRotator& OutRotation) const;

	void OnMontageBlendingOutStarted(UAnimMontage* AnimMontage, bool bInterrupted);
	void OnMontageEnded(UAnimMontage* AnimMontage, bool bInterrupted);

	UFUNCTION()
	void OnMeleeTraceHit(UMeleeTraceComponent* TraceComp, AActor* HitActor, const FVector& HitLocation,
	                     const FVector& HitNormal, FName HitBoneName, FMeleeTraceInstanceHandle TraceHandle);

	UPROPERTY(Transient)
	TObjectPtr<UACAction> Source;

	UPROPERTY(Transient)
	TObjectPtr<UACActionDataAsset> DataAsset;

	UPROPERTY(Transient)
	TObjectPtr<ACharacter> Owner;

	UPROPERTY(Transient)
	TObjectPtr<UACActionComponent> OwnerComponent;

	UPROPERTY(Transient)
	TObjectPtr<UACCharacterMovementComponent> CharacterMovementComponent;

	UPROPERTY(Transient)
	TObjectPtr<UACAnimInstance> AnimInstance;

	UPROPERTY(Transient)
	TObjectPtr<UMeleeTraceComponent> MeleeTraceComponent;

	EACActionInstanceState State = EACActionInstanceState::None;
	int32 MontageInstanceID = INDEX_NONE;
	bool bMoveCancelable = false;
	bool bActionCancelable = false;
	bool bMovementLocked = false;
	bool bHitDetecting = false;
	bool bLunging = false;
	FACHitEffect CurrentHitEffect;
};
