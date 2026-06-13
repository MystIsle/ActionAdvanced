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

namespace
{
	// 메시 지터가 히트 방향 축에서 벗어나는 랜덤 콘 반각(도).
	constexpr float GMeshShakeConeAngleDeg = 25.f;
}

UACHitReactionComponent::UACHitReactionComponent()
{
	// 메시 셰이크 동안만 틱을 켠다(상시 틱 아님).
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UACHitReactionComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (OwnerCharacter)
	{
		MovementComponent = OwnerCharacter->GetCharacterMovement<UACCharacterMovementComponent>();
		SkeletalMesh = OwnerCharacter->GetMesh();
	}
}

void UACHitReactionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 리액션 도중 파괴/레벨전환/빙의교체 시 락·애님레이트·FX 딜레이션이 재사용 CMC/메시/FX에 남지 않도록 정리.
	if (bReacting)
	{
		CancelReact();
	}
	StopMeshShake();
	if (SkeletalMesh)
	{
		SkeletalMesh->GlobalAnimRateScale = 1.f;
	}
	RestoreFXTimeScale();
	EndHitFlash();

	Super::EndPlay(EndPlayReason);
}

void UACHitReactionComponent::PlayReact(const FACHitInfo& HitInfo)
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

	const FACHitEffect& Effect = HitInfo.Effect;

	// 경직: 입력 잠금 + AI 두뇌 일시정지
	if (MovementComponent)
	{
		MovementComponent->SetMovementLocked(true);
	}
	SetBrainLogicPaused(true);

	FaceAttacker(HitInfo.Direction);

	const int32 ReactMontageInstanceID = PlayHitReactMontage();
	SpawnHitFX(Effect, HitInfo.Direction, HitInfo.HitLocation, ReactMontageInstanceID);

	// 넉백 파라미터 보관(히트스탑 후 StartKnockback에서 사용)
	PendingKnockbackDir = HitInfo.Direction.GetSafeNormal2D();
	PendingKnockbackDistance = Effect.KnockbackDistance;
	PendingKnockbackDuration = Effect.KnockbackDuration;
	PendingKnockbackCurve = Effect.KnockbackEaseCurve;

	// 히트스탑(단일 진입점) — 피격 몽타주 인스턴스 타겟
	RequestHitStop(Effect.HitStopPlayRate, Effect.HitStopDuration, ReactMontageInstanceID);

	// 메시 지터(히트스톱 지속에 동기화). Amplitude 0 또는 히트스톱 없으면 no-op.
	StartMeshShake(Effect.MeshShakeAmplitude, Effect.MeshShakeSampleCount, Effect.HitStopDuration, HitInfo.Direction);

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

	UAnimInstance* AnimInstance = SkeletalMesh ? SkeletalMesh->GetAnimInstance() : nullptr;
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
	StopMeshShake(); // 재피격 전 base 복원(상위 PlayReact가 새로 재캐시)
	FinishReact();
}

void UACHitReactionComponent::RequestHitStop(float Scale, float Duration, int32 MontageInstanceID)
{
	if (Duration <= 0.f || OwnerCharacter == nullptr || SkeletalMesh == nullptr)
	{
		return;
	}

	SkeletalMesh->GlobalAnimRateScale = Scale;

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
	if (SkeletalMesh)
	{
		SkeletalMesh->GlobalAnimRateScale = 1.f;
	}
	RestoreFXTimeScale();
	StopMeshShake(); // 히트스톱 종료 = 셰이크 종료(동기화 지점)
}

void UACHitReactionComponent::RestoreFXTimeScale()
{
	if (ScaledFXController.IsValid())
	{
		ScaledFXController->SetFXTimeScale(1.f);
	}
	ScaledFXController = nullptr;
}

void UACHitReactionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateMeshShake();
}

void UACHitReactionComponent::UpdateMeshShake()
{
	if (bMeshShaking == false || SkeletalMesh == nullptr)
	{
		return;
	}

	// 히트스톱은 anim rate만 늦출 뿐 월드시간·틱은 실시간 → 경과시간 기반은 정상 동작.
	const float Elapsed = GetWorld()->GetTimeSeconds() - MeshShakeStartTime;

	// 샘플 간격마다 새 방향: 히트 축을 따라 +/- 교대 + 약간의 랜덤 콘(스텝형 버즈).
	if (MeshShakeSampleInterval > 0.f && Elapsed >= MeshShakeNextSampleTime)
	{
		const FVector ConeDir = FMath::VRandCone(MeshShakeBaseDir, FMath::DegreesToRadians(GMeshShakeConeAngleDeg));
		MeshShakeCurrentDir = ConeDir * MeshShakeSign;
		MeshShakeSign = -MeshShakeSign;
		MeshShakeNextSampleTime += MeshShakeSampleInterval;
	}

	// 선형 감쇠 엔벨로프(끝에서 base로 수렴).
	const float Envelope = MeshShakeDuration > 0.f ? FMath::Max(0.f, 1.f - Elapsed / MeshShakeDuration) : 0.f;
	const FVector Offset = MeshShakeCurrentDir * (MeshShakeAmplitude * Envelope);

	SkeletalMesh->SetRelativeLocation(MeshShakeBaseRelativeLocation + Offset);
}

void UACHitReactionComponent::StartMeshShake(float Amplitude, int32 SampleCount, float Duration, const FVector& Direction)
{
	if (SkeletalMesh == nullptr || Amplitude <= 0.f || Duration <= 0.f || SampleCount <= 0)
	{
		return;
	}

	// 진행 중이면 먼저 base 복원(지터된 트랜스폼을 새 base로 캐시하는 사고 방지).
	if (bMeshShaking)
	{
		SkeletalMesh->SetRelativeLocation(MeshShakeBaseRelativeLocation);
	}

	MeshShakeBaseRelativeLocation = SkeletalMesh->GetRelativeLocation();
	MeshShakeBaseDir = Direction.GetSafeNormal2D();
	if (MeshShakeBaseDir.IsNearlyZero())
	{
		MeshShakeBaseDir = FVector::ForwardVector; // 방향 없으면 폴백
	}
	MeshShakeAmplitude = Amplitude;
	MeshShakeSampleInterval = Duration / SampleCount;
	MeshShakeDuration = Duration;
	MeshShakeStartTime = GetWorld()->GetTimeSeconds();
	MeshShakeNextSampleTime = 0.f;
	MeshShakeSign = 1.f;
	MeshShakeCurrentDir = FVector::ZeroVector;
	bMeshShaking = true;

	SetComponentTickEnabled(true);
}

void UACHitReactionComponent::StopMeshShake()
{
	if (bMeshShaking == false)
	{
		return;
	}

	bMeshShaking = false;
	SetComponentTickEnabled(false);

	if (SkeletalMesh)
	{
		SkeletalMesh->SetRelativeLocation(MeshShakeBaseRelativeLocation);
	}
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

	if (SkeletalMesh == nullptr)
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
	SkeletalMesh->SetOverlayMaterial(FlashMID);

	OwnerCharacter->GetWorldTimerManager().SetTimer(
		FlashTimer, this, &UACHitReactionComponent::EndHitFlash, Settings->FlashDuration, false);
}

void UACHitReactionComponent::EndHitFlash()
{
	if (OwnerCharacter)
	{
		OwnerCharacter->GetWorldTimerManager().ClearTimer(FlashTimer);
	}
	if (SkeletalMesh)
	{
		SkeletalMesh->SetOverlayMaterial(nullptr);
	}
}

UACMontageInstanceController* UACHitReactionComponent::FindMontageInstanceController(int32 MontageInstanceID) const
{
	if (MontageInstanceID == INDEX_NONE || SkeletalMesh == nullptr)
	{
		return nullptr;
	}

	UACAnimInstance* AnimInstance = Cast<UACAnimInstance>(SkeletalMesh->GetAnimInstance());
	return AnimInstance ? AnimInstance->FindMontageInstanceController(MontageInstanceID) : nullptr;
}
