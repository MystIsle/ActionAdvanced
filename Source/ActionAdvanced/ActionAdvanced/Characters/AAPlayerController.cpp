// Fill out your copyright notice in the Description page of Project Settings.


#include "AAPlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

void AAAPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	auto InputComp = Cast<UEnhancedInputComponent>(InputComponent);
	if (InputComp == nullptr)
	{
		return;
	}

	InputComp->BindAction(MoveInputAction, ETriggerEvent::Triggered, this, &AAAPlayerController::OnMoveInput);
}

void AAAPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (auto EnhancedInputSubsystem = GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
	{
		EnhancedInputSubsystem->AddMappingContext(InputMappingContext, 0);
	}
}

void AAAPlayerController::OnUnPossess()
{
	Super::OnUnPossess();

	if (auto EnhancedInputSubsystem = GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
	{
		EnhancedInputSubsystem->RemoveMappingContext(InputMappingContext);
	}
}

void AAAPlayerController::OnMoveInput(const FInputActionValue& Value)
{
	APawn* ControlledPawn = GetPawn();
	if (ControlledPawn == nullptr)
	{
		return;
	}

	const FVector2D Axis = Value.Get<FVector2D>();
	const FRotator YawRotation(0.0f, GetControlRotation().Yaw, 0.0f);
	const FVector ForwardDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	ControlledPawn->AddMovementInput(ForwardDir, Axis.Y);
	ControlledPawn->AddMovementInput(RightDir, Axis.X);
}
