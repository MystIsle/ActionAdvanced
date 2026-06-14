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
};
