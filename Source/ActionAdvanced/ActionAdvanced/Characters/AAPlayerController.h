// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "AAPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;
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
	void OnInputMoveTriggered(const FInputActionValue& Value);
	void OnInputLookTriggered(const FInputActionValue& Value);
	void OnInputJumpStarted();
	void OnInputSprint(const FInputActionValue& Value);
	void OnInputLightAttackStarted();
	void OnInputHeavyAttackStarted();

protected:
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UInputMappingContext> InputMappingContext;
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UInputAction> MoveInputAction;
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UInputAction> LookInputAction;
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UInputAction> JumpInputAction;
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UInputAction> SprintInputAction;
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UInputAction> LightAttackInputAction;
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UInputAction> HeavyAttackInputAction;
};
