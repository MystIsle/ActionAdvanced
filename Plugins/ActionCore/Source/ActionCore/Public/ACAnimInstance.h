// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "ACAnimInstance.generated.h"

class UACMontageInstanceController;
class UAnimMontage;

/**
 * 몽타주 인스턴스 컨트롤러의 생성/보관/조회 담당.
 * 캐릭터 ABP는 이 클래스를 부모로 해야 한다 — 액션 시스템의 필수 전제(UACActionInstance::Initialize에서 검증).
 */
UCLASS()
class ACTIONCORE_API UACAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUninitializeAnimation() override;
	
	UACMontageInstanceController* FindMontageInstanceController(int32 InMontageInstanceID) const;
	void NotifyMontageInstanceControllerFinished(const UACMontageInstanceController* Controller);

private:
	UFUNCTION()
	void HandleMontageStarted(UAnimMontage* Montage);

	UPROPERTY(Transient)
	TMap<int32, TObjectPtr<UACMontageInstanceController>> Controllers;
};
