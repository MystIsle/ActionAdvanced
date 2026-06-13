// Fill out your copyright notice in the Description page of Project Settings.


#include "ACActionComponent.h"

#include "ACAction.h"
#include "ACActionDataAsset.h"
#include "ACActionInstance.h"
#include "ActionCore.h"
#include "GameFramework/Character.h"


UACActionComponent::UACActionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bWantsInitializeComponent = true;
}

void UACActionComponent::InitializeComponent()
{
	Super::InitializeComponent();

	OwnerCharacter = GetOwner<ACharacter>();
	ensure(OwnerCharacter);

	for (UACActionDataAsset* DataAsset : DataAssets)
	{
		if (DataAsset == nullptr)
		{
			UE_LOG(LogActionCore, Error, TEXT("[%s] DataAssets에 nullptr 항목이 있어 건너뜁니다."),
			       *GetNameSafe(GetOwner()));
			continue;
		}

		// IsDataValid는 에디터 전용이므로, 쿠킹/런타임 빌드에서도 무효 에셋이 맵에 등록되지 않도록 방어한다.
		if (DataAsset->Key.IsValid() == false || DataAsset->AnimMontage == nullptr)
		{
			UE_LOG(LogActionCore, Error, TEXT("[%s] 무효한 DataAsset이 있어 건너뜁니다. (DataAsset: %s, Key: %s, Montage: %s)"),
			       *GetNameSafe(GetOwner()), *GetNameSafe(DataAsset), *DataAsset->Key.ToString(), *GetNameSafe(DataAsset->AnimMontage));
			continue;
		}

		UACAction* Action = CreateAction(DataAsset);

		const FGameplayTag ActionKey = Action->GetKey();
		if (const TObjectPtr<UACAction>* ExistingAction = Actions.Find(ActionKey))
		{
			UE_LOG(LogActionCore, Warning, TEXT("[%s] 중복된 ActionKey가 있어 액션 등록을 건너뜁니다. (ActionKey: %s, ExistingAction: %s, SkippedAction: %s)"),
			       *GetNameSafe(GetOwner()), *ActionKey.ToString(), *GetNameSafe(ExistingAction->Get()), *GetNameSafe(Action));
			continue;
		}

		Actions.Add(Action->GetKey(), Action);
	}
}
void UACActionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (ActivateInstances.IsEmpty())
	{
		return;
	}

	TArray<TObjectPtr<UACActionInstance> > Instances;
	ActivateInstances.GenerateValueArray(Instances);

	for (UACActionInstance* Instance : Instances)
	{
		check(Instance);

		//NOTE: 몽타주 인스턴스의 Stop을 호출 보장해줘야, 인스턴스에 의존하는 효과들의 온-오프가 맞아떨어진다.
		Instance->Stop();
	}
}

UACActionInstance* UACActionComponent::PlayAction(FGameplayTag ActionKey, const FRotator& InRotation)
{
	auto Result = Actions.Find(ActionKey);
	if (Result == nullptr)
	{
		UE_LOG(LogActionCore, Warning, TEXT("[%s] ActionKey를 찾을 수 없어 액션을 실행할 수 없습니다. (ActionKey: %s)"),
		       *GetNameSafe(GetOwner()), *ActionKey.ToString());
		return nullptr;
	}

	UACAction* Action = *Result;
	if (Action == nullptr)
	{
		UE_LOG(LogActionCore, Error, TEXT("[%s] ActionKey에 매핑된 Action이 nullptr입니다. (ActionKey: %s)"),
		       *GetNameSafe(GetOwner()), *ActionKey.ToString());
		return nullptr;
	}

	return Action->Play(InRotation);
}

void UACActionComponent::StopActiveAction()
{
	if (PlayingInstance == nullptr)
	{
		return;
	}
	
	PlayingInstance->Stop();
	PlayingInstance = nullptr;
}

bool UACActionComponent::CanPlayAction(const UACAction* InAction) const
{
	check(InAction);
	//NOTE: 올 캔슬 액션 등의 특수한 경우 - 재생중인 액션과 액션의 트랜지션 조건 등을 여기서 판별함.

	return PlayingInstance == nullptr || PlayingInstance->IsCancelable();
}

UACActionInstance* UACActionComponent::GetActivateInstance(int32 MontageInstanceID) const
{
	return ActivateInstances.FindRef(MontageInstanceID);
}

void UACActionComponent::NotifyActionInstancePlayed(UACActionInstance* InInstance)
{
	check(InInstance);
	
	if (PlayingInstance)
	{
		PlayingInstance->Stop();
	}

	PlayingInstance = InInstance;

	const int32 MontageInstanceID = InInstance->GetMontageInstanceID();
	const bool bAlreadyActivated = ActivateInstances.Contains(MontageInstanceID);
	ActivateInstances.Add(MontageInstanceID, InInstance);

	UE_LOG(LogActionCore, Verbose, TEXT("[%s] 활성 액션 인스턴스 추가: %s (MontageInstanceID: %d, bAlreadyActivated: %s, ActiveCount: %d)"),
	       *GetNameSafe(GetOwner()), *GetNameSafe(InInstance), MontageInstanceID, bAlreadyActivated ? TEXT("true") : TEXT("false"), ActivateInstances.Num());
}

void UACActionComponent::NotifyActionInstanceStopped(UACActionInstance* InInstance)
{
	check(InInstance);
	if (PlayingInstance == InInstance)
	{
		PlayingInstance = nullptr;
	}
}

void UACActionComponent::NotifyActionInstanceEnded(UACActionInstance* InInstance)
{
	check(InInstance);

	const int32 MontageInstanceID = InInstance->GetMontageInstanceID();
	const int32 RemovedCount = ActivateInstances.Remove(MontageInstanceID);

	UE_LOG(LogActionCore, Verbose, TEXT("[%s] 활성 액션 인스턴스 제거: %s (MontageInstanceID: %d, RemovedCount: %d, ActiveCount: %d)"),
	       *GetNameSafe(GetOwner()), *GetNameSafe(InInstance), MontageInstanceID, RemovedCount, ActivateInstances.Num());
}

// ReSharper disable once CppMemberFunctionMayBeStatic
UACAction* UACActionComponent::CreateAction(UACActionDataAsset* DataAsset)
{
	check(DataAsset);
	UACAction* Action = NewObject<UACAction>(this);
	Action->Initialize(this, DataAsset);
	return Action;
}
