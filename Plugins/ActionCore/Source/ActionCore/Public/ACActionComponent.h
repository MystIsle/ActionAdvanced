// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "ACActionComponent.generated.h"


class UACActionDataAsset;
class UACAction;
class UACActionInstance;

// 재생 중 액션이 끝나 컴포넌트가 유휴(PlayingInstance 없음)로 돌아올 때 브로드캐스트.
// 콤보 체이닝(다음 액션이 인터럽트)으로 끝난 경우엔 발화하지 않는다.
DECLARE_MULTICAST_DELEGATE(FACOnReturnedToIdle);

UCLASS()
class ACTIONCORE_API UACActionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UACActionComponent();

	virtual void InitializeComponent() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UACActionInstance* PlayAction(FGameplayTag ActionKey, const FRotator& InRotation);
	void StopActiveAction();
	bool CanPlayAction(const UACAction* InAction) const;

	UACActionInstance* GetPlayingInstance() const { return PlayingInstance; }
	UACActionInstance* GetActivateInstance(int32 MontageInstanceID) const;

	void NotifyActionInstancePlayed(UACActionInstance* InInstance);
	void NotifyActionInstanceStopped(UACActionInstance* InInstance);
	void NotifyActionInstanceEnded(UACActionInstance* InInstance);

	// 콤보가 전이를 허가한 시점에 진행 중 액션의 캔슬 게이트를 연다(컨트롤러가 호출).
	void MarkPlayingActionCancelable();

	FACOnReturnedToIdle OnReturnedToIdle;

private:
	UACAction* CreateAction(UACActionDataAsset* DataAsset);

protected:
	UPROPERTY(EditDefaultsOnly)
	TArray<TObjectPtr<UACActionDataAsset> > DataAssets;

	UPROPERTY(VisibleInstanceOnly, Transient)
	TMap<FGameplayTag, TObjectPtr<UACAction> > Actions;

	UPROPERTY(Transient)
	TObjectPtr<ACharacter> OwnerCharacter;

	UPROPERTY(Transient)
	TObjectPtr<UACActionInstance> PlayingInstance;

	UPROPERTY(Transient)
	TMap<int32, TObjectPtr<UACActionInstance> > ActivateInstances;
};
