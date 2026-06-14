// Fill out your copyright notice in the Description page of Project Settings.


#include "AAPlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputActionValue.h"
#include "ACActionComponent.h"
#include "ACCharacterMovementComponent.h"
#include "Components/ComboHandlerComponent.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"
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
		CachedComboComponent->OnComboWindowOpened.AddUObject(this, &AAAPlayerController::TryConsumeBuffer);
	}

	if (CachedActionComponent)
	{
		CachedActionComponent->OnReturnedToIdle.AddUObject(this, &AAAPlayerController::ScheduleDrainNextTick);
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
		CachedComboComponent->OnComboWindowOpened.RemoveAll(this);
		CachedComboComponent = nullptr;
	}

	if (CachedActionComponent)
	{
		CachedActionComponent->OnReturnedToIdle.RemoveAll(this);
	}

	BufferedInput = nullptr;
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

	// 이동 캔슬 = 콤보 이탈: 이동이 실제로 먹는 상태(이동락 해제)에서 유효 입력이 들어오면 콤보 플로우를 끊는다.
	// 진행 중 공격 몽타주는 그대로 두고 콤보 상태만 리셋 → 다음 공격 입력은 새 Opener(1타).
	if (CachedComboComponent && CachedComboComponent->GetIsComboActive())
	{
		if (const UACCharacterMovementComponent* CMC = Cast<UACCharacterMovementComponent>(ControlledPawn->GetMovementComponent()))
		{
			if (CMC->IsMovementInputLocked() == false)
			{
				CachedComboComponent->NotifyComboActionEnded();
			}
		}
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
	ACharacter* ControlledCharacter = Cast<ACharacter>(GetPawn());
	if (ControlledCharacter == nullptr)
	{
		return;
	}

	// 점프도 이동의 일부 — 액션이 이동을 잠근 동안(캔슬 윈도우 전)엔 막는다.
	if (const UACCharacterMovementComponent* CMC = Cast<UACCharacterMovementComponent>(ControlledCharacter->GetCharacterMovement()))
	{
		if (CMC->IsMovementInputLocked())
		{
			return;
		}
	}

	ControlledCharacter->Jump();
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
	BufferInput(LightAttackInputAction);
}

void AAAPlayerController::OnInputHeavyAttackStarted()
{
	BufferInput(HeavyAttackInputAction);
}

void AAAPlayerController::OnComboPlayAction(FGameplayTag ActionTag)
{
	if (CachedActionComponent)
	{
		// 콤보 그래프가 이 전이를 허가했으므로, 진행 중 타를 캔슬해 새 타가 인터럽트하도록 게이트를 연다.
		CachedActionComponent->MarkPlayingActionCancelable();
		CachedActionComponent->PlayAction(ActionTag, ActionRotation);
	}
}

void AAAPlayerController::BufferInput(UInputAction* InputAction)
{
	if (CachedComboComponent == nullptr)
	{
		return;
	}

	BufferedInput = InputAction;
	BufferedTime = GetWorld()->GetTimeSeconds();
	TryConsumeBuffer();
}

void AAAPlayerController::TryConsumeBuffer()
{
	if (BufferedInput == nullptr || CachedComboComponent == nullptr)
	{
		return;
	}

	// 만료된 입력은 폐기(콤보 의도가 지난 입력).
	if (GetWorld()->GetTimeSeconds() - BufferedTime > InputBufferWindow)
	{
		BufferedInput = nullptr;
		return;
	}

	// 발동 성공 시에만 비운다(lazy). 막히면 보존해 다음 시점에 재시도.
	if (CachedComboComponent->ProcessComboAttack(BufferedInput))
	{
		BufferedInput = nullptr;
	}
}

void AAAPlayerController::ScheduleDrainNextTick()
{
	// ③ 유휴 복귀 드레인 전용. 콤보 리셋(NotifyComboActionEnded)도 OnReturnedToIdle 구독이라, 동기로
	// 부르면 등록 순서에 따라 리셋보다 먼저 돌 수 있다. 다음 틱으로 미뤄 "리셋 후 발동"을 보장한다.
	if (BufferedInput && GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &AAAPlayerController::TryConsumeBuffer);
	}
}
