// Fill out your copyright notice in the Description page of Project Settings.

#include "ACAnimNotifyState_TimedNiagara.h"

#include "ACAnimInstance.h"
#include "ACMontageInstanceController.h"
#include "Animation/ActiveMontageInstanceScope.h"
#include "Components/SkeletalMeshComponent.h"
#include "NiagaraComponent.h"

void UACAnimNotifyState_TimedNiagara::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                                  const float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	if (MeshComp == nullptr)
	{
		return;
	}

	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	UNiagaraComponent* NiagaraComponent = Cast<UNiagaraComponent>(GetSpawnedEffect(MeshComp));
	if (NiagaraComponent == nullptr)
	{
		return;
	}

	const UE::Anim::FAnimNotifyMontageInstanceContext* MontageContext =
		EventReference.GetContextData<UE::Anim::FAnimNotifyMontageInstanceContext>();
	if (MontageContext == nullptr)
	{
		return;
	}

	UACAnimInstance* AnimInstance = Cast<UACAnimInstance>(MeshComp->GetAnimInstance());
	if (AnimInstance == nullptr)
	{
		return;
	}

	UACMontageInstanceController* Controller = AnimInstance->FindMontageInstanceController(MontageContext->MontageInstanceID);
	if (Controller == nullptr)
	{
		return;
	}

	FACMontageFXEntry Entry;
	Entry.Component = NiagaraComponent;
	Entry.bFollowAnimTimeScale = bFollowAnimTimeScale;
	Controller->RegisterFX(Entry);
}
