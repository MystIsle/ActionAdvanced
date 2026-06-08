// Fill out your copyright notice in the Description page of Project Settings.


#include "ACAction.h"

#include "ACActionComponent.h"
#include "ACActionDataAsset.h"
#include "ACActionInstance.h"
#include "ActionCore.h"

UACAction::UACAction()
{
}

void UACAction::Initialize(UACActionComponent* InComponent, UACActionDataAsset* InDataAsset)
{
	check(InComponent);
	check(InDataAsset);
	
	OwnerComponent = InComponent;
	DataAsset = InDataAsset;
	Key = DataAsset->Key;
}

bool UACAction::CanPlay() const
{
	return OwnerComponent->CanPlayAction(this);
}

UACActionInstance* UACAction::Play(const FRotator& InRotation)
{
	if (CanPlay() == false)
	{
		UE_LOG(LogActionCore, Verbose, TEXT("[%s] 재생 조건을 충족하지 못해 액션을 실행하지 않습니다. (Key: %s)"),
			*GetName(), *Key.ToString());
		return nullptr;
	}
	
	UACActionInstance* ActionInstance = NewObject<UACActionInstance>(this);
	ActionInstance->Initialize(this);
	if (ActionInstance->GetState() != EACActionInstanceState::Ready)
	{
		return nullptr;
	}
	
	if (ActionInstance->Play(InRotation) == false)
	{
		return nullptr;
	}

	return ActionInstance;
}
