// Fill out your copyright notice in the Description page of Project Settings.


#include "AAPlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputActionValue.h"
#include "ACActionComponent.h"
#include "Components/ComboHandlerComponent.h"
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

	InputComp->BindAction(MoveInputAction, ETriggerEvent::Triggered, this, &AAAPlayerController::OnInputMove);
	InputComp->BindAction(MoveInputAction, ETriggerEvent::Completed, this, &AAAPlayerController::OnInputMove);
	InputComp->BindAction(LookInputAction, ETriggerEvent::Triggered, this, &AAAPlayerController::OnInputLookTriggered);
	InputComp->BindAction(JumpInputAction, ETriggerEvent::Started, this, &AAAPlayerController::OnInputJumpStarted);
	InputComp->BindAction(SprintInputAction, ETriggerEvent::Started, this, &AAAPlayerController::OnInputSprint);
	InputComp->BindAction(SprintInputAction, ETriggerEvent::Completed, this, &AAAPlayerController::OnInputSprint);
	InputComp->BindAction(LightAttackInputAction, ETriggerEvent::Started, this,
	                      &AAAPlayerController::OnInputLightAttackStarted);
	InputComp->BindAction(HeavyAttackInputAction, ETriggerEvent::Started, this,
	                      &AAAPlayerController::OnInputHeavyAttackStarted);
}

void AAAPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	CachedActionComponent = InPawn ? InPawn->FindComponentByClass<UACActionComponent>() : nullptr;

	CachedComboComponent = InPawn ? InPawn->FindComponentByClass<UComboHandlerComponent>() : nullptr;
	if (CachedComboComponent)
	{
		CachedComboComponent->OnPlayComboAction.AddDynamic(this, &AAAPlayerController::OnComboPlayAction);
	}

	// 매핑 컨텍스트는 로컬 컨트롤러 전용 작업이다. 원격/비로컬 컨트롤러에서는 GetLocalPlayer()가 null이므로 가드한다.
	if (IsLocalPlayerController())
	{
		if (auto* EnhancedInputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(
			GetLocalPlayer()))
		{
			EnhancedInputSubsystem->AddMappingContext(InputMappingContext, 0);
		}
	}

	ActionRotation = InPawn->GetActorRotation();
}

void AAAPlayerController::OnUnPossess()
{
	Super::OnUnPossess();

	if (CachedComboComponent)
	{
		CachedComboComponent->OnPlayComboAction.RemoveDynamic(this, &AAAPlayerController::OnComboPlayAction);
		CachedComboComponent = nullptr;
	}

	CachedActionComponent = nullptr;

	if (IsLocalPlayerController())
	{
		if (auto* EnhancedInputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(
			GetLocalPlayer()))
		{
			EnhancedInputSubsystem->RemoveMappingContext(InputMappingContext);
		}
	}
}

FGenericTeamId AAAPlayerController::GetGenericTeamId() const
{
	if (const IGenericTeamAgentInterface* TeamPawn = Cast<IGenericTeamAgentInterface>(GetPawn()))
	{
		return TeamPawn->GetGenericTeamId();
	}
	return FGenericTeamId::NoTeam;
}

void AAAPlayerController::PlayAction(FGameplayTag ActionTag)
{
	if (CachedActionComponent == nullptr || ActionTag.IsValid() == false)
	{
		return;
	}

	CachedActionComponent->PlayAction(ActionTag, ActionRotation);
}

void AAAPlayerController::OnInputMove(const FInputActionInstance& Instance)
{
	APawn* ControlledPawn = GetPawn();
	if (ControlledPawn == nullptr)
	{
		return;
	}

	if (Instance.GetTriggerEvent() != ETriggerEvent::Triggered)
	{
		ActionRotation = ControlledPawn->GetActorRotation();
		return;
	}

	const FVector2D Axis = Instance.GetValue().Get<FVector2D>();
	const FRotator YawRotation(0.0f, GetControlRotation().Yaw, 0.0f);
	const FVector ForwardDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	ControlledPawn->AddMovementInput(ForwardDir, Axis.Y);
	ControlledPawn->AddMovementInput(RightDir, Axis.X);

	if (Axis.IsNearlyZero())
	{
		ActionRotation = ControlledPawn->GetActorRotation();
		return;
	}

	const FVector InputDir = FVector(Axis.Y, Axis.X, 0.f).GetSafeNormal();
	const FVector WorldInputDir = YawRotation.RotateVector(InputDir);
	ActionRotation = WorldInputDir.Rotation();
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
	UE_LOG(LogActionAdvanced, Log, TEXT("OnInputSprint - 미구현 스텁 (%s)"),
	       bWantsToSprint ? TEXT("press") : TEXT("release"));
}

void AAAPlayerController::OnInputLightAttackStarted()
{
	if (CachedComboComponent)
	{
		CachedComboComponent->ProcessComboAttack(LightAttackInputAction);
	}
}

void AAAPlayerController::OnInputHeavyAttackStarted()
{
	if (CachedComboComponent)
	{
		CachedComboComponent->ProcessComboAttack(HeavyAttackInputAction);
	}
}

void AAAPlayerController::OnComboPlayAction(FGameplayTag ActionTag)
{
	if (CachedActionComponent)
	{
		CachedActionComponent->PlayAction(ActionTag, ActionRotation);
	}
}
