// Fill out your copyright notice in the Description page of Project Settings.

#include "ACMontageInstanceController.h"

#include "ACAnimInstance.h"
#include "Animation/AnimMontage.h"
#include "NiagaraComponent.h"

void UACMontageInstanceController::Initialize(FAnimMontageInstance& MontageInstance)
{
	MontageInstanceID = MontageInstance.GetInstanceID();

	MontageInstance.OnMontageBlendingOutStarted.BindUObject(this, &ThisClass::HandleBlendingOutStarted);
	MontageInstance.OnMontageEnded.BindUObject(this, &ThisClass::HandleEnded);
}

void UACMontageInstanceController::RegisterFX(const FACMontageFXEntry& Entry)
{
	if (Entry.Component.IsValid() == false)
	{
		return;
	}

	const FACMontageFXEntry& Added = FXEntries.Add_GetRef(Entry);
	const float Applied = FMath::Max(CurrentTimeScale, Added.MinTimeScale);
	if (Added.bFollowAnimTimeScale && Applied != 1.f)
	{
		Added.Component->SetCustomTimeDilation(Applied);
	}
}

void UACMontageInstanceController::SetFXTimeScale(const float Scale)
{
	CurrentTimeScale = Scale;

	for (int32 Index = FXEntries.Num() - 1; Index >= 0; --Index)
	{
		UNiagaraComponent* Component = FXEntries[Index].Component.Get();
		if (Component && FXEntries[Index].bFollowAnimTimeScale)
		{
			// 하한 적용: 복원(1.0) 시에는 Max(1, 하한)=1이라 별도 분기 불필요.
			Component->SetCustomTimeDilation(FMath::Max(Scale, FXEntries[Index].MinTimeScale));
		}
	}
}

void UACMontageInstanceController::HandleBlendingOutStarted(UAnimMontage* Montage, bool bInterrupted)
{
	OnBlendingOutStarted.Broadcast(Montage, bInterrupted);
}

void UACMontageInstanceController::HandleEnded(UAnimMontage* Montage, bool bInterrupted)
{
	OnEnded.Broadcast(Montage, bInterrupted);

	// 몽타주 인스턴스 종료 = 컨트롤러 수명 종료: FX 롤백 후 소유자 맵에서 제거.
	SetFXTimeScale(1.f);
	if (UACAnimInstance* OwnerAnimInstance = GetTypedOuter<UACAnimInstance>())
	{
		OwnerAnimInstance->NotifyMontageInstanceControllerFinished(this);
	}
}
