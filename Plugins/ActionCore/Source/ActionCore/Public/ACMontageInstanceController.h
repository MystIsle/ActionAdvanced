// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ACMontageInstanceController.generated.h"

class UAnimMontage;
class UNiagaraComponent;
struct FAnimMontageInstance;

USTRUCT()
struct ACTIONCORE_API FACMontageFXEntry
{
	GENERATED_BODY()

	TWeakObjectPtr<UNiagaraComponent> Component;

	bool bFollowAnimTimeScale = true;

	// 타임스케일 하한(0~1). 0보다 크면 히트스탑 동안에도 최소 이 속도로 재생(슬로모).
	float MinTimeScale = 0.f;
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FACOnMontageBlendingOutStarted, UAnimMontage*, bool /*bInterrupted*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FACOnMontageEnded, UAnimMontage*, bool /*bInterrupted*/);

UCLASS()
class ACTIONCORE_API UACMontageInstanceController : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(FAnimMontageInstance& MontageInstance);

	int32 GetMontageInstanceID() const { return MontageInstanceID; }

	void RegisterFX(const FACMontageFXEntry& Entry);
	void SetFXTimeScale(float Scale);

	FACOnMontageBlendingOutStarted OnBlendingOutStarted;
	FACOnMontageEnded OnEnded;

private:
	void HandleBlendingOutStarted(UAnimMontage* Montage, bool bInterrupted);
	void HandleEnded(UAnimMontage* Montage, bool bInterrupted);

	TArray<FACMontageFXEntry> FXEntries;
	int32 MontageInstanceID = INDEX_NONE;
	float CurrentTimeScale = 1.f;
};
