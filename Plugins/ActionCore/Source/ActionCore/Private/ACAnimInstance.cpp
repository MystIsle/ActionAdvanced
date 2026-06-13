// Fill out your copyright notice in the Description page of Project Settings.

#include "ACAnimInstance.h"

#include "ACMontageInstanceController.h"
#include "Animation/AnimMontage.h"

void UACAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	OnMontageStarted.AddUniqueDynamic(this, &ThisClass::HandleMontageStarted);
}

void UACAnimInstance::NativeUninitializeAnimation()
{
	// 안전망: Ended 콜백 없이 사라지는 경로(레벨 전환 등)에서 FX 딜레이션이 남지 않게.
	for (const TPair<int32, TObjectPtr<UACMontageInstanceController>>& Pair : Controllers)
	{
		if (Pair.Value)
		{
			Pair.Value->SetFXTimeScale(1.f);
		}
	}
	Controllers.Empty();

	Super::NativeUninitializeAnimation();
}

void UACAnimInstance::HandleMontageStarted(UAnimMontage* Montage)
{
	// OnMontageStarted는 Montage_Play 내부 동기 브로드캐스트 → PlayAnimMontage 리턴 전에 컨트롤러 존재 보장.
	FAnimMontageInstance* MontageInstance = GetActiveInstanceForMontage(Montage);
	if (MontageInstance == nullptr)
	{
		return;
	}

	UACMontageInstanceController* Controller = NewObject<UACMontageInstanceController>(this);
	Controller->Initialize(*MontageInstance);
	Controllers.Add(Controller->GetMontageInstanceID(), Controller);
}

UACMontageInstanceController* UACAnimInstance::FindMontageInstanceController(const int32 InMontageInstanceID) const
{
	return Controllers.FindRef(InMontageInstanceID);
}

void UACAnimInstance::NotifyMontageInstanceControllerFinished(const UACMontageInstanceController* Controller)
{
	if (Controller)
	{
		Controllers.Remove(Controller->GetMontageInstanceID());
	}
}
