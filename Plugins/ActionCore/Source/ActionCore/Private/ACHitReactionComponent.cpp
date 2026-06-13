// Fill out your copyright notice in the Description page of Project Settings.

#include "ACHitReactionComponent.h"

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

	// 경직: 입력 잠금 + AI 두뇌 일시정지
	if (MovementComponent)
	{
		MovementComponent->SetMovementLocked(true);
	}
	SetBrainLogicPaused(true);

	FaceAttacker(Direction);

	const int32 ReactMontageInstanceID = PlayHitReactMontage();
	SpawnHitFX(Effect, Direction, HitLocation, ReactMontageInstanceID);

	// 넉백 파라미터 보관(히트스탑 후 StartKnockback에서 사용)
	PendingKnockbackDir = Direction.GetSafeNormal2D();
	PendingKnockbackDistance = Effect.KnockbackDistance;
	PendingKnockbackDuration = Effect.KnockbackDuration;
	PendingKnockbackCurve = Effect.KnockbackEaseCurve;

	// 히트스탑(단일 진입점) — 피격 몽타주 인스턴스 타겟
	RequestHitStop(Effect.HitStopPlayRate, Effect.HitStopDuration, ReactMontageInstanceID);

	// 넉백 타이밍: 히트스탑 후
	if (Effect.HitStopDuration > 0.f)
	{
		OwnerCharacter->GetWorldTimerManager().SetTimer(
			ReactTimer, this, &UACHitReactionComponent::StartKnockback, Effect.HitStopDuration, false);
	}
	else
	{
		StartKnockback();
	}

	// 히트 점멸(전역 세팅, Effect 무관)
	StartHitFlash();
}

void UACHitReactionComponent::FaceAttacker(const FVector& HitDirection)
{
	// 넉백 반대방향(때린 쪽)을 즉시 응시 (보간 없음).
	const FVector FaceDir = (-HitDirection).GetSafeNormal2D();
	if (!FaceDir.IsNearlyZero())
	{
		OwnerCharacter->SetActorRotation(FaceDir.Rotation());
	}
}

int32 UACHitReactionComponent::PlayHitReactMontage()
{
	// 피격 몽타주 배열 순차 재생(끝나면 0으로 순환) + 인스턴스ID 취득(FX/히트스탑 타겟).
	if (HitReactMontages.Num() == 0)
	{
		return INDEX_NONE;
	}

	const int32 Index = HitMontageIndex % HitReactMontages.Num();
	HitMontageIndex = (HitMontageIndex + 1) % HitReactMontages.Num();

	UAnimMontage* Montage = HitReactMontages[Index];
	if (Montage == nullptr || OwnerCharacter->PlayAnimMontage(Montage) <= 0.f)
	{
		return INDEX_NONE;
	}

	USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh();
	UAnimInstance* AnimInstance = Mesh ? Mesh->GetAnimInstance() : nullptr;
	const FAnimMontageInstance* MontageInstance =
		AnimInstance ? AnimInstance->GetActiveInstanceForMontage(Montage) : nullptr;
	return MontageInstance ? MontageInstance->GetInstanceID() : INDEX_NONE;
}

void UACHitReactionComponent::SpawnHitFX(const FACHitEffect& Effect, const FVector& Direction, const FVector& HitLocation, int32 MontageInstanceID)
{
	// 히트 위치에 비부착 스폰(때린 쪽을 향함) 후 피격 몽타주 컨트롤러에 등록 → 히트스탑 연동.
	if (Effect.HitNiagara == nullptr)
	{
		return;
	}

	const FRotator FXRotation = (-Direction).GetSafeNormal().Rotation();
	UNiagaraComponent* HitFXComponent =
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), Effect.HitNiagara, HitLocation, FXRotation, Effect.HitFXScale);
	if (HitFXComponent == nullptr)
	{
		return;
	}

	// 프리롤은 등록(딜레이션 적용) 전에: 시작 프레임을 피크로 옮긴 뒤 동결/슬로모.
	if (Effect.bHitFXPreSim && Effect.HitFXPreSimTime > 0.f)
	{
		HitFXComponent->AdvanceSimulationByTime(Effect.HitFXPreSimTime, 1.f / 60.f);
	}

	if (UACMontageInstanceController* Controller = FindMontageInstanceController(MontageInstanceID))
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

void UACHitReactionComponent::SetBrainLogicPaused(bool bPaused)
{
	AAIController* AI = OwnerCharacter ? Cast<AAIController>(OwnerCharacter->GetController()) : nullptr;
	UBrainComponent* Brain = AI ? AI->GetBrainComponent() : nullptr;
	if (Brain == nullptr)
	{
		return;
	}

	if (bPaused)
	{
		Brain->PauseLogic(TEXT("HitReact"));
	}
	else
	{
		Brain->ResumeLogic(TEXT("HitReact"));
	}
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
	SetBrainLogicPaused(false);
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

void UACHitReactionComponent::RestoreFXTimeScale()
{
	if (ScaledFXController.IsValid())
	{
		ScaledFXController->SetFXTimeScale(1.f);
	}
	ScaledFXController = nullptr;
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

	if (FlashMID == nullptr || FlashMID->Parent != FlashMaterial)
	{
		FlashMID = UMaterialInstanceDynamic::Create(FlashMaterial, this);
	}

	FlashMID->SetVectorParameterValue(TEXT("FlashColor"), Settings->FlashColor);
	FlashMID->SetScalarParameterValue(TEXT("FlashStartTime"), GetWorld()->GetTimeSeconds());
	FlashMID->SetScalarParameterValue(TEXT("FlashDuration"), Settings->FlashDuration);
	FlashMID->SetScalarParameterValue(TEXT("FlashIntensity"), Settings->FlashIntensity);
	FlashMID->SetScalarParameterValue(TEXT("FlashExponent"), Settings->FlashExponent);
	Mesh->SetOverlayMaterial(FlashMID);

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
