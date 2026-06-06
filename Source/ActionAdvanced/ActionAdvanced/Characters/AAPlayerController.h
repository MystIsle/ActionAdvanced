// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/PlayerController.h"
#include "AAPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;
class UACActionComponent;
struct FInputActionValue;

UCLASS()
class ACTIONADVANCED_API AAAPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void SetupInputComponent() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

protected:
	void PlayAction(FGameplayTag ActionTag);
	
	void OnInputMoveTriggered(const FInputActionValue& Value);
	void OnInputLookTriggered(const FInputActionValue& Value);
	void OnInputJumpStarted();
	void OnInputSprint(const FInputActionValue& Value);
	void OnInputLightAttackStarted();
	void OnInputHeavyAttackStarted();

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
	
	FRotator ActionRotation;
};
