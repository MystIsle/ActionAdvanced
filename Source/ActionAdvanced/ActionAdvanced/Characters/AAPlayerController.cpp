// Fill out your copyright notice in the Description page of Project Settings.


#include "AAPlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "GameFramework/Character.h"
#include "ActionAdvanced.h"

void AAAPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* InputComp = Cast<UEnhancedInputComponent>(InputComponent);
	if (InputComp == nullptr)
	{
		return;
	}

	InputComp->BindAction(MoveInputAction, ETriggerEvent::Triggered, this, &AAAPlayerController::OnInputMoveTriggered);
	InputComp->BindAction(LookInputAction, ETriggerEvent::Triggered, this, &AAAPlayerController::OnInputLookTriggered);
	InputComp->BindAction(JumpInputAction, ETriggerEvent::Started, this, &AAAPlayerController::OnInputJumpStarted);
	InputComp->BindAction(SprintInputAction, ETriggerEvent::Started, this, &AAAPlayerController::OnInputSprint);
	InputComp->BindAction(SprintInputAction, ETriggerEvent::Completed, this, &AAAPlayerController::OnInputSprint);
	InputComp->BindAction(LightAttackInputAction, ETriggerEvent::Started, this, &AAAPlayerController::OnInputLightAttackStarted);
	InputComp->BindAction(HeavyAttackInputAction, ETriggerEvent::Started, this, &AAAPlayerController::OnInputHeavyAttackStarted);
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

void AAAPlayerController::OnInputMoveTriggered(const FInputActionValue& Value)
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

void AAAPlayerController::OnInputLookTriggered(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();

	// 상하 반전이 필요하면 IMC의 Negate 모디파이어로 처리한다.
	AddYawInput(Axis.X);
	AddPitchInput(Axis.Y);
}

void AAAPlayerController::OnInputJumpStarted()
{
	if (ACharacter* ControlledCharacter = Cast<ACharacter>(GetPawn()))
	{
		ControlledCharacter->Jump();
	}
}

void AAAPlayerController::OnInputSprint(const FInputActionValue& Value)
{
	// Started에서 true, Completed에서 false가 들어온다 (홀드형 입력).
	const bool bWantsToSprint = Value.Get<bool>();

	// TODO: AACharacter에 스프린트(MaxWalkSpeed 토글) 구현 후 연결한다.
	UE_LOG(LogActionAdvanced, Log, TEXT("OnInputSprint - 미구현 스텁 (%s)"), bWantsToSprint ? TEXT("press") : TEXT("release"));
}

void AAAPlayerController::OnInputLightAttackStarted()
{
	// TODO: AACharacter에 공격 진입점 구현 후 연결한다.
	UE_LOG(LogActionAdvanced, Log, TEXT("OnInputLightAttackStarted - 미구현 스텁"));
}

void AAAPlayerController::OnInputHeavyAttackStarted()
{
	// TODO: AACharacter에 공격 진입점 구현 후 연결한다.
	UE_LOG(LogActionAdvanced, Log, TEXT("OnInputHeavyAttackStarted - 미구현 스텁"));
}
