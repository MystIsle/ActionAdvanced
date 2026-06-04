// Fill out your copyright notice in the Description page of Project Settings.


#include "ACActionContext.h"

#include "ACActionComponent.h"
#include "ACActionInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"

FACActionContext::FACActionContext(const FBranchingPointNotifyPayload& Payload)
{
	USkeletalMeshComponent* Mesh = Payload.SkelMeshComponent;
	if (Mesh == nullptr)
	{
		return;
	}

	ACharacter* OwnerCharacter = Cast<ACharacter>(Mesh->GetOwner());
	if (OwnerCharacter == nullptr)
	{
		return;
	}

	UACActionComponent* Component = OwnerCharacter->FindComponentByClass<UACActionComponent>();
	if (Component == nullptr)
	{
		return;
	}

	Owner = OwnerCharacter;
	Instance = Component->GetActivateInstance(Payload.MontageInstanceID);
}
