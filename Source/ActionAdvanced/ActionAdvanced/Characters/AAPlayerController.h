// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/PlayerController.h"
#include "GenericTeamAgentInterface.h"
#include "AAPlayerController.generated.h"

struct FInputActionInstance;
class UInputAction;
class UInputMappingContext;
class UACActionComponent;
class UComboHandlerComponent;
struct FInputActionValue;

UCLASS()
class ACTIONADVANCED_API AAAPlayerController : public APlayerController, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	virtual FGenericTeamId GetGenericTeamId() const override;

protected:
	virtual void SetupInputComponent() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

protected:
	void PlayAction(FGameplayTag ActionTag);
	
	void OnInputMove(const FInputActionInstance& Instance);
	void OnInputLookTriggered(const FInputActionValue& Value);
	void OnInputJumpStarted();
	void OnInputSprint(const FInputActionValue& Value);
	void OnInputLightAttackStarted();
	void OnInputHeavyAttackStarted();

	// ComboHandler가 다음 콤보 노드의 ActionTag를 통보 → 우리 액션 시스템으로 재생.
	UFUNCTION()
	void OnComboPlayAction(FGameplayTag ActionTag);

	// 선입력 버퍼: 입력을 스탬프하고, 발동 가능 시점(입력 즉시 / 콤보창 열림 / 유휴 복귀)에 소비.
	// 재진입은 무관(노티·몽타주 이벤트는 UE가 Advance 밖 batch 단계에서 부름). 단 ③ 유휴는 콤보 리셋과
	// 같은 OnReturnedToIdle 구독이라, 리셋 후 발동을 보장하려 next-tick으로만 미룬다(②는 동기).
	void BufferInput(UInputAction* InputAction);
	void TryConsumeBuffer();
	void ScheduleDrainNextTick();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> InputMappingContext;
	
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveInputAction;
	
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> LookInputAction;
	
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> JumpInputAction;
	
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> SprintInputAction;
	
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> LightAttackInputAction;
	
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> HeavyAttackInputAction;

	UPROPERTY(EditDefaultsOnly, Category = "Action")
	FGameplayTag LightAttackTag;

	UPROPERTY(EditDefaultsOnly, Category = "Action")
	FGameplayTag HeavyAttackTag;

	UPROPERTY(Transient)
	TObjectPtr<UACActionComponent> CachedActionComponent;

	UPROPERTY(Transient)
	TObjectPtr<UComboHandlerComponent> CachedComboComponent;

	FRotator ActionRotation;

	// 선입력 버퍼(단일 슬롯). 회전은 발동 시점 ActionRotation을 쓰므로 버퍼에 담지 않는다.
	UPROPERTY(EditDefaultsOnly, Category = "Combo", meta = (ClampMin = "0.0"))
	float InputBufferWindow = 0.15f;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> BufferedInput;

	double BufferedTime = -1.0;
};
