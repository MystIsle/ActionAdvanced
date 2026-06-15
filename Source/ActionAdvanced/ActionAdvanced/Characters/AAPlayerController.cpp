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
#include "ACCombatFeelSettings.h"
#include "Engine/Engine.h"
#include "InputCoreTypes.h"
#include "HAL/IConsoleManager.h"

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

#if !UE_BUILD_SHIPPING
	SetupFeelDebugInput();
#endif
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

	// 이동 캔슬 = 콤보 이탈: 이동이 실제로 먹는 상태(이동락 해제)에서 유효 입력이 들어오면 콤보를 끊고
	// 진행 공격도 멈춰 이동으로 전환한다. 루트모션 공격은 몽타주가 살아있으면 velocity를 점유해 이동을
	// 덮어쓰므로, StopActiveAction으로 블렌드아웃시켜 루트모션을 풀어야 입력이 먹는다.
	if (CachedComboComponent && CachedComboComponent->GetIsComboActive())
	{
		if (const UACCharacterMovementComponent* CMC = Cast<UACCharacterMovementComponent>(ControlledPawn->GetMovementComponent()))
		{
			if (CMC->IsMovementInputLocked() == false)
			{
				CachedComboComponent->NotifyComboActionEnded();
				if (CachedActionComponent)
				{
					CachedActionComponent->StopActiveAction();
				}
				// 이동을 택했으니 직전에 버퍼된 공격 의사도 폐기 — 0.15s 내 유령 1타 방지.
				BufferedInput = nullptr;
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
	if (BufferedInput)
	{
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &AAAPlayerController::TryConsumeBuffer);
	}
}

#if !UE_BUILD_SHIPPING
static void ToggleBoolCVar(const TCHAR* Name)
{
	if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(Name))
	{
		Var->Set(Var->GetInt() == 0 ? TEXT("1") : TEXT("0"), ECVF_SetByConsole);
	}
}

static bool GetBoolCVar(const TCHAR* Name)
{
	const IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(Name);
	return Var && Var->GetInt() != 0;
}

void AAAPlayerController::SetupFeelDebugInput()
{
	if (InputComponent == nullptr)
	{
		return;
	}

	// EnhancedInput과 별개의 레거시 BindKey라 별도 IA 에셋 없이 바로 동작(셰이핑 빌드에서는 컴파일 제외).
	InputComponent->BindKey(EKeys::One,   IE_Pressed, this, &AAAPlayerController::OnToggleFeelMaster);
	InputComponent->BindKey(EKeys::Two,   IE_Pressed, this, &AAAPlayerController::OnToggleFeelHitStop);
	InputComponent->BindKey(EKeys::Three, IE_Pressed, this, &AAAPlayerController::OnToggleFeelHitFX);
	InputComponent->BindKey(EKeys::Four,  IE_Pressed, this, &AAAPlayerController::OnToggleFeelFlash);
	InputComponent->BindKey(EKeys::Five,  IE_Pressed, this, &AAAPlayerController::OnToggleFeelCameraShake);
	InputComponent->BindKey(EKeys::Six,   IE_Pressed, this, &AAAPlayerController::OnToggleFeelMeshShake);
	InputComponent->BindKey(EKeys::Seven, IE_Pressed, this, &AAAPlayerController::OnToggleMeleeTraceDebug);
	InputComponent->BindKey(EKeys::Eight, IE_Pressed, this, &AAAPlayerController::OnToggleAutoTargetDebug);
	InputComponent->BindKey(EKeys::Zero,  IE_Pressed, this, &AAAPlayerController::OnToggleFeelOverlay);
}

void AAAPlayerController::OnToggleFeelMaster()      { UACCombatFeelSettings::ToggleFeelMaster(); }
void AAAPlayerController::OnToggleFeelHitStop()     { UACCombatFeelSettings::ToggleFeelLayer(EACFeelLayer::HitStop); }
void AAAPlayerController::OnToggleFeelHitFX()       { UACCombatFeelSettings::ToggleFeelLayer(EACFeelLayer::HitFX); }
void AAAPlayerController::OnToggleFeelFlash()       { UACCombatFeelSettings::ToggleFeelLayer(EACFeelLayer::Flash); }
void AAAPlayerController::OnToggleFeelCameraShake() { UACCombatFeelSettings::ToggleFeelLayer(EACFeelLayer::CameraShake); }
void AAAPlayerController::OnToggleFeelMeshShake()   { UACCombatFeelSettings::ToggleFeelLayer(EACFeelLayer::MeshShake); }
void AAAPlayerController::OnToggleFeelOverlay()     { bShowFeelOverlay = !bShowFeelOverlay; }
void AAAPlayerController::OnToggleMeleeTraceDebug() { ToggleBoolCVar(TEXT("MeleeTrace.ShouldDrawDebug")); }
void AAAPlayerController::OnToggleAutoTargetDebug() { ToggleBoolCVar(TEXT("AA.AutoTarget.Debug")); }

void AAAPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (IsLocalPlayerController())
	{
		DrawFeelDebugOverlay();
	}
}

void AAAPlayerController::DrawFeelDebugOverlay() const
{
	if (bShowFeelOverlay == false || GEngine == nullptr)
	{
		return;
	}

	const bool bMaster = UACCombatFeelSettings::GetFeelMaster();

	// 슬롯 키가 작을수록 위. 2~6 레이어 / 7701 마스터 / 7700 헤더.
	auto DrawLayer = [&](int32 Slot, int32 KeyNum, EACFeelLayer Layer)
	{
		const bool bRaw = UACCombatFeelSettings::GetFeelLayerRaw(Layer);
		const FColor Color = bMaster == false ? FColor(120, 120, 120) : (bRaw ? FColor::Green : FColor::Red);
		GEngine->AddOnScreenDebugMessage(7700 + Slot, 0.f, Color,
			FString::Printf(TEXT("  [%d] %-11s : %s"), KeyNum,
				UACCombatFeelSettings::GetFeelLayerLabel(Layer), bRaw ? TEXT("ON") : TEXT("OFF")));
	};

	DrawLayer(6, 6, EACFeelLayer::MeshShake);
	DrawLayer(5, 5, EACFeelLayer::CameraShake);
	DrawLayer(4, 4, EACFeelLayer::Flash);
	DrawLayer(3, 3, EACFeelLayer::HitFX);
	DrawLayer(2, 2, EACFeelLayer::HitStop);

	GEngine->AddOnScreenDebugMessage(7701, 0.f, bMaster ? FColor::Green : FColor::Red,
		FString::Printf(TEXT("  [1] %-11s : %s"), TEXT("Master"), bMaster ? TEXT("ON") : TEXT("OFF")));
	GEngine->AddOnScreenDebugMessage(7700, 0.f, FColor::Cyan,
		TEXT("== 연출 토글  (1~6: 토글 / 0: 표시 on-off) =="));

	// 디버그 시각화(개발용) — MeleeTrace/AutoTarget은 ENABLE_DRAW_DEBUG 빌드에서만 실제 그려진다.
	auto DrawCVarRow = [&](int32 Slot, int32 KeyNum, const TCHAR* Label, const TCHAR* CVarName)
	{
		const bool bOn = GetBoolCVar(CVarName);
		GEngine->AddOnScreenDebugMessage(7700 + Slot, 0.f, bOn ? FColor::Green : FColor::Red,
			FString::Printf(TEXT("  [%d] %-11s : %s"), KeyNum, Label, bOn ? TEXT("ON") : TEXT("OFF")));
	};
	DrawCVarRow(12, 8, TEXT("AutoTarget"), TEXT("AA.AutoTarget.Debug"));
	DrawCVarRow(11, 7, TEXT("MeleeTrace"), TEXT("MeleeTrace.ShouldDrawDebug"));
	GEngine->AddOnScreenDebugMessage(7710, 0.f, FColor::Cyan, TEXT("-- 디버그 표시 (7~8) --"));
}
#endif
