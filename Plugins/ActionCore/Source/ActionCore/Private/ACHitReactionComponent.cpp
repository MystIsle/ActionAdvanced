// Fill out your copyright notice in the Description page of Project Settings.

#include "ACHitReactionComponent.h"

#include "ACActionComponent.h"
#include "ACActionInstance.h"
#include "ACAnimInstance.h"
#include "ACCombatFeelSettings.h"
#include "ACMontageInstanceController.h"
#include "ACCharacterMovementComponent.h"
#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "BrainComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Curves/CurveFloat.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/RootMotionSource.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "TimerManager.h"

UACHitReactionComponent::UACHitReactionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UACHitReactionComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (OwnerCharacter)
	{
		MovementComponent = OwnerCharacter->GetCharacterMovement<UACCharacterMovementComponent>();
	}
}

void UACHitReactionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 리액션 도중 파괴/레벨전환/빙의교체 시 락·애님레이트·FX 딜레이션이 재사용 CMC/메시/FX에 남지 않도록 정리.
	if (bReacting)
	{
		CancelReact();
	}
	if (OwnerCharacter)
	{
		if (USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh())
		{
			Mesh->GlobalAnimRateScale = 1.f;
		}
	}
	RestoreFXTimeScale();
	EndHitFlash();

	Super::EndPlay(EndPlayReason);
}

void UACHitReactionComponent::PlayReact(const FACHitEffect& Effect, const FVector& Direction, const FVector& HitLocation)
{
	if (OwnerCharacter == nullptr)
	{
		return;
	}

	if (bReacting)
	{
		CancelReact(); // 재피격: 진행 중 리액션 리셋
	}
	bReacting = true;

	// 1. 진행 중 공격 중단
	if (UACActionComponent* ActionComp = OwnerCharacter->FindComponentByClass<UACActionComponent>())
	{
		if (UACActionInstance* Playing = ActionComp->GetPlayingInstance())
		{
			Playing->Stop();
		}
	}

	// 2. 경직(입력 잠금) + AI면 BrainComponent 일시정지
	if (MovementComponent)
	{
		MovementComponent->SetMovementLocked(true);
	}
	if (AAIController* AI = Cast<AAIController>(OwnerCharacter->GetController()))
	{
		if (UBrainComponent* Brain = AI->GetBrainComponent())
		{
			Brain->PauseLogic(TEXT("HitReact"));
		}
	}

	// 넉백 반대방향(때린 쪽)을 즉시 응시 (보간 없음).
	const FVector FaceDir = (-Direction).GetSafeNormal2D();
	if (!FaceDir.IsNearlyZero())
	{
		OwnerCharacter->SetActorRotation(FaceDir.Rotation());
	}

	// 3. 피격 몽타주 (배열 순차 재생, 끝나면 0으로 순환) + 인스턴스ID 취득(FX/히트스탑 타겟)
	int32 ReactMontageInstanceID = INDEX_NONE;
	if (HitReactMontages.Num() > 0)
	{
		const int32 Index = HitMontageIndex % HitReactMontages.Num();
		HitMontageIndex = (HitMontageIndex + 1) % HitReactMontages.Num();
		if (UAnimMontage* Montage = HitReactMontages[Index])
		{
			if (OwnerCharacter->PlayAnimMontage(Montage) > 0.f)
			{
				USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh();
				UAnimInstance* AnimInstance = Mesh ? Mesh->GetAnimInstance() : nullptr;
				const FAnimMontageInstance* MontageInstance =
					AnimInstance ? AnimInstance->GetActiveInstanceForMontage(Montage) : nullptr;
				if (MontageInstance)
				{
					ReactMontageInstanceID = MontageInstance->GetInstanceID();
				}
			}
		}
	}

	// 4. 히트 FX: 히트 위치에 비부착 스폰(때린 쪽을 향함) 후 피격 몽타주 컨트롤러에 등록 → 히트스탑 연동.
	if (Effect.HitNiagara)
	{
		const FRotator FXRotation = (-Direction).GetSafeNormal().Rotation();
		if (UNiagaraComponent* HitFXComponent =
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), Effect.HitNiagara, HitLocation, FXRotation,
			                                               Effect.HitFXScale))
		{
			// 프리롤은 등록(딜레이션 적용) 전에: 시작 프레임을 피크로 옮긴 뒤 동결/슬로모.
			if (Effect.bHitFXPreSim && Effect.HitFXPreSimTime > 0.f)
			{
				HitFXComponent->AdvanceSimulationByTime(Effect.HitFXPreSimTime, 1.f / 60.f);
			}

			if (UACMontageInstanceController* Controller = FindMontageInstanceController(ReactMontageInstanceID))
			{
				FACMontageFXEntry Entry;
				Entry.Component = HitFXComponent;
				if (Effect.bHitFXMinTimeScale)
				{
					Entry.MinTimeScale = Effect.HitFXMinTimeScale;
				}
				Controller->RegisterFX(Entry);
			}
		}
	}

	// 넉백 파라미터 보관(히트스탑 후 StartKnockback에서 사용)
	PendingKnockbackDir = Direction.GetSafeNormal2D();
	PendingKnockbackDistance = Effect.KnockbackDistance;
	PendingKnockbackDuration = Effect.KnockbackDuration;
	PendingKnockbackCurve = Effect.KnockbackEaseCurve;

	// 5. 히트스탑(단일 진입점) — 피격 몽타주 인스턴스 타겟
	RequestHitStop(Effect.HitStopPlayRate, Effect.HitStopDuration, ReactMontageInstanceID);

	// 6. 넉백 타이밍: 히트스탑 후
	if (Effect.HitStopDuration > 0.f)
	{
		OwnerCharacter->GetWorldTimerManager().SetTimer(
			ReactTimer, this, &UACHitReactionComponent::StartKnockback, Effect.HitStopDuration, false);
	}
	else
	{
		StartKnockback();
	}

	// 7. 히트 점멸(전역 세팅, Effect 무관)
	StartHitFlash();
}

void UACHitReactionComponent::RequestHitStop(float Scale, float Duration, int32 MontageInstanceID)
{
	if (Duration <= 0.f || OwnerCharacter == nullptr)
	{
		return;
	}

	USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh();
	if (Mesh == nullptr)
	{
		return;
	}

	Mesh->GlobalAnimRateScale = Scale;

	// FX 타겟 교체 시 이전 타겟 먼저 복원(마지막이 이김 — 안 하면 이전 몽타주 FX가 종료까지 얼어붙음).
	UACMontageInstanceController* Target = FindMontageInstanceController(MontageInstanceID);
	if (ScaledFXController.IsValid() && ScaledFXController.Get() != Target)
	{
		ScaledFXController->SetFXTimeScale(1.f);
	}
	ScaledFXController = Target;
	if (Target)
	{
		Target->SetFXTimeScale(Scale);
	}

	// 재진입 시 타이머 리셋(마지막이 이김)
	OwnerCharacter->GetWorldTimerManager().SetTimer(
		HitStopTimer, this, &UACHitReactionComponent::OnHitStopFinished, Duration, false);
}

void UACHitReactionComponent::OnHitStopFinished()
{
	if (OwnerCharacter)
	{
		if (USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh())
		{
			Mesh->GlobalAnimRateScale = 1.f;
		}
	}
	RestoreFXTimeScale();
}

void UACHitReactionComponent::StartHitFlash()
{
	if (OwnerCharacter == nullptr)
	{
		return;
	}

	const UACCombatFeelSettings* Settings = GetDefault<UACCombatFeelSettings>();
	if (Settings == nullptr || Settings->bEnableHitFlash == false || Settings->FlashDuration <= 0.f)
	{
		return;
	}

	UMaterialInterface* FlashMaterial = Settings->FlashMaterial.LoadSynchronous();
	if (FlashMaterial == nullptr)
	{
		return;
	}

	USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh();
	if (Mesh == nullptr)
	{
		return;
	}

	// MID는 1회 생성 후 재사용(매 피격 Create 회피). 소스 머티리얼이 바뀌면 재생성.
	if (FlashMID == nullptr || FlashMID->Parent != FlashMaterial)
	{
		FlashMID = UMaterialInstanceDynamic::Create(FlashMaterial, this);
	}

	// falloff는 머티리얼이 (Time - FlashStartTime)/FlashDuration으로 자체 계산 → 틱 불필요.
	FlashMID->SetVectorParameterValue(TEXT("FlashColor"), Settings->FlashColor);
	FlashMID->SetScalarParameterValue(TEXT("FlashStartTime"), GetWorld()->GetTimeSeconds());
	FlashMID->SetScalarParameterValue(TEXT("FlashDuration"), Settings->FlashDuration);
	FlashMID->SetScalarParameterValue(TEXT("FlashIntensity"), Settings->FlashIntensity);
	FlashMID->SetScalarParameterValue(TEXT("FlashExponent"), Settings->FlashExponent);
	Mesh->SetOverlayMaterial(FlashMID);

	// 감쇠 종료 시 오버레이 해제(재피격 시 SetTimer가 리셋 — 마지막이 이김).
	OwnerCharacter->GetWorldTimerManager().SetTimer(
		FlashTimer, this, &UACHitReactionComponent::EndHitFlash, Settings->FlashDuration, false);
}

void UACHitReactionComponent::EndHitFlash()
{
	if (OwnerCharacter)
	{
		OwnerCharacter->GetWorldTimerManager().ClearTimer(FlashTimer);
		if (USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh())
		{
			Mesh->SetOverlayMaterial(nullptr);
		}
	}
	// FlashMID는 보존(재사용). 다음 StartHitFlash에서 갱신.
}

void UACHitReactionComponent::RestoreFXTimeScale()
{
	if (ScaledFXController.IsValid())
	{
		ScaledFXController->SetFXTimeScale(1.f);
	}
	ScaledFXController = nullptr;
}

UACMontageInstanceController* UACHitReactionComponent::FindMontageInstanceController(int32 MontageInstanceID) const
{
	if (MontageInstanceID == INDEX_NONE || OwnerCharacter == nullptr)
	{
		return nullptr;
	}

	USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh();
	UACAnimInstance* AnimInstance = Mesh ? Cast<UACAnimInstance>(Mesh->GetAnimInstance()) : nullptr;
	return AnimInstance ? AnimInstance->FindMontageInstanceController(MontageInstanceID) : nullptr;
}

void UACHitReactionComponent::StartKnockback()
{
	if (OwnerCharacter == nullptr || MovementComponent == nullptr || PendingKnockbackDistance <= 0.f)
	{
		FinishReact();
		return;
	}

	const FVector Start = OwnerCharacter->GetActorLocation();
	const FVector Target = Start + PendingKnockbackDir * PendingKnockbackDistance;

	TSharedPtr<FRootMotionSource_MoveToDynamicForce> Move = MakeShared<FRootMotionSource_MoveToDynamicForce>();
	Move->InstanceName = TEXT("HitKnockback");
	Move->AccumulateMode = ERootMotionAccumulateMode::Override;
	Move->Priority = 10;
	Move->StartLocation = Start;
	Move->InitialTargetLocation = Target;
	Move->SetTargetLocation(Target);
	Move->Duration = PendingKnockbackDuration;
	Move->bRestrictSpeedToExpected = true;
	Move->TimeMappingCurve = PendingKnockbackCurve; // 이징(null이면 등속)
	Move->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::SetVelocity;
	Move->FinishVelocityParams.SetVelocity = FVector::ZeroVector;

	KnockbackSourceID = MovementComponent->ApplyRootMotionSource(Move);

	OwnerCharacter->GetWorldTimerManager().SetTimer(
		ReactTimer, this, &UACHitReactionComponent::FinishReact, PendingKnockbackDuration, false);
}

void UACHitReactionComponent::FinishReact()
{
	if (MovementComponent)
	{
		MovementComponent->SetMovementLocked(false);
	}
	if (OwnerCharacter)
	{
		if (AAIController* AI = Cast<AAIController>(OwnerCharacter->GetController()))
		{
			if (UBrainComponent* Brain = AI->GetBrainComponent())
			{
				Brain->ResumeLogic(TEXT("HitReact"));
			}
		}
	}
	bReacting = false;
	KnockbackSourceID = 0;
}

void UACHitReactionComponent::CancelReact()
{
	if (OwnerCharacter)
	{
		OwnerCharacter->GetWorldTimerManager().ClearTimer(ReactTimer);
	}
	if (MovementComponent && KnockbackSourceID != 0)
	{
		MovementComponent->RemoveRootMotionSourceByID(KnockbackSourceID);
	}
	FinishReact();
}
