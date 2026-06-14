// Fill out your copyright notice in the Description page of Project Settings.


#include "ACActionInstance.h"

#include "ACAction.h"
#include "ACActionComponent.h"
#include "ACActionDataAsset.h"
#include "ACAnimInstance.h"
#include "ACMontageInstanceController.h"
#include "ACCharacterMovementComponent.h"
#include "ACEnumLibrary.h"
#include "ActionCore.h"
#include "MotionWarpingComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "ACTargetingLibrary.h"
#include "GenericTeamAgentInterface.h"
#include "ACHitReactionComponent.h"
#include "ACHittable.h"

#if ENABLE_DRAW_DEBUG
#include "DrawDebugHelpers.h"
#endif

void UACActionInstance::Initialize(UACAction* InAction)
{
	if (InAction == nullptr)
	{
		return;
	}

	Source = InAction;
	DataAsset = InAction->GetDataAsset();
	OwnerComponent = InAction->GetOwnerComponent();

	Owner = OwnerComponent->GetOwner<ACharacter>();
	if (ensure(Owner) == false)
	{
		return;
	}

	// CMC가 플러그인 타입이 아니면 Play()에서 무가드로 역참조하므로, 여기서 막아 Ready 전이를 차단한다.
	CharacterMovementComponent = Owner->GetCharacterMovement<UACCharacterMovementComponent>();
	if (ensure(CharacterMovementComponent) == false)
	{
		return;
	}

	USkeletalMeshComponent* Mesh = Owner->GetMesh();
	if (ensure(Mesh) == false)
	{
		return;
	}

	// ABP가 UACAnimInstance가 아니면 몽타주 콜백(블렌드아웃/종료)을 받을 수 없으므로 Ready 전이를 차단한다.
	AnimInstance = Cast<UACAnimInstance>(Mesh->GetAnimInstance());
	if (ensureMsgf(AnimInstance, TEXT("ABP가 UACAnimInstance를 상속해야 합니다. 리페런팅 필요 (Owner: %s)"), *GetNameSafe(Owner)) == false)
	{
		return;
	}

	MeleeTraceComponent = Owner->FindComponentByClass<UMeleeTraceComponent>();

	SetState(EACActionInstanceState::Ready);
}

bool UACActionInstance::Play(const FRotator& InRotation)
{
	//TODO: 관성화로 느낌 살릴 수 있게 :: 즉시 끊되 불연속 짧게 흡수(???)
	const float Duration = Owner->PlayAnimMontage(DataAsset->AnimMontage);
	if (Duration <= 0.f)
	{
		UE_LOG(LogActionCore, Warning, TEXT("[%s] 애님 몽타주 재생에 실패했습니다. (Owner: %s, Montage: %s)"),
		       *GetName(), *GetNameSafe(Owner), *GetNameSafe(DataAsset->AnimMontage));
		return IsPlaying();
	}

	if (CharacterMovementComponent->IsFalling() == false)
	{
		CharacterMovementComponent->StopMovementImmediately();
	}
	SetMovementLocked(true);

	FAnimMontageInstance* MontageInstance = AnimInstance->GetActiveInstanceForMontage(DataAsset->AnimMontage);
	check(MontageInstance);
	MontageInstanceID = MontageInstance->GetInstanceID();

	// OnMontageStarted 동기 브로드캐스트로 이 시점 컨트롤러 존재가 보장된다(ACAnimInstance::HandleMontageStarted).
	UACMontageInstanceController* Controller = GetMontageInstanceController();
	check(Controller);
	Controller->OnBlendingOutStarted.AddUObject(this, &ThisClass::OnMontageBlendingOutStarted);
	Controller->OnEnded.AddUObject(this, &ThisClass::OnMontageEnded);

	// 워프 목표: 오토타게팅 러시가 잡히면 타겟 앞 지점, 아니면 제자리 + 회전(폴백).
	FVector WarpLocation;
	FRotator WarpRotation;
	const bool bLunging = TryLungeWarp(InRotation, WarpLocation, WarpRotation);
	if (bLunging == false)
	{
		WarpLocation = Owner->GetActorLocation();
		WarpRotation = DetermineFacingRotation(InRotation);
	}

	if (const auto MotionWarpingComp = Owner->GetComponentByClass<UMotionWarpingComponent>())
	{
		static const FName WarpTargetName("Target");
		MotionWarpingComp->AddOrUpdateWarpTargetFromLocationAndRotation(WarpTargetName, WarpLocation, WarpRotation);
	}
	else
	{
		UE_LOG(LogActionCore, Warning, TEXT("[%s] 모션워핑 컴포넌트가 존재하지 않습니다. (Owner: %s, Montage: %s)"),
		       *GetName(), *GetNameSafe(Owner), *GetNameSafe(DataAsset->AnimMontage));
	}

	// 공중에선 루트모션을 끄되, 러시(데이터에셋 활성 + 타겟 찾음)는 공중에서도 워프가 먹도록 유지.
	if (CharacterMovementComponent->IsFalling() && bLunging == false)
	{
		MontageInstance->PushDisableRootMotion();
	}

	SetState(EACActionInstanceState::Playing);

	OwnerComponent->NotifyActionInstancePlayed(this);

	return IsPlaying();
}

void UACActionInstance::Stop()
{
	StopInternal(true);
}

bool UACActionInstance::IsActionCancelable() const
{
	return bActionCancelable;
}

void UACActionInstance::MarkMoveCancelable()
{
	if (bMoveCancelable)
	{
		return;
	}

	bMoveCancelable = true;
	SetMovementLocked(false);
}

void UACActionInstance::SetActionCancelable(bool bInCancelable)
{
	bActionCancelable = bInCancelable;
}

void UACActionInstance::BeginHitDetection(const FACHitEffect& InHitEffect)
{
	if (bHitDetecting || MeleeTraceComponent == nullptr)
	{
		return;
	}

	CurrentHitEffect = InHitEffect;
	bHitDetecting = true;
	MeleeTraceComponent->OnTraceHit.AddDynamic(this, &ThisClass::OnMeleeTraceHit);
}

void UACActionInstance::EndHitDetection()
{
	if (bHitDetecting == false || MeleeTraceComponent == nullptr)
	{
		return;
	}

	bHitDetecting = false;
	MeleeTraceComponent->OnTraceHit.RemoveDynamic(this, &ThisClass::OnMeleeTraceHit);
}

void UACActionInstance::OnMeleeTraceHit(UMeleeTraceComponent* TraceComp, AActor* HitActor, const FVector& HitLocation,
                                        const FVector& HitNormal, FName HitBoneName, FMeleeTraceInstanceHandle TraceHandle)
{
	if (HitActor == nullptr || FGenericTeamId::GetAttitude(Owner, HitActor) != ETeamAttitude::Hostile)
	{
		return;
	}

	UE_LOG(LogActionCore, Log, TEXT("[Melee] Hit %s (bone %s)"), *GetNameSafe(HitActor), *HitBoneName.ToString());

	//NOTE: 현재 편의상 넉백 방향은, 한방향으로만.
	const FVector Direction = Owner->GetActorForwardVector();

	if (IACHittable* Hittable = Cast<IACHittable>(HitActor))
	{
		FACHitInfo HitInfo;
		HitInfo.Effect = CurrentHitEffect;
		HitInfo.Direction = Direction;
		HitInfo.HitLocation = HitLocation;
		
		Hittable->ReceiveHit(HitInfo);
	}

	if (UACHitReactionComponent* SelfReaction = Owner->FindComponentByClass<UACHitReactionComponent>())
	{
		SelfReaction->RequestHitStop(CurrentHitEffect.HitStopPlayRate, CurrentHitEffect.HitStopDuration, MontageInstanceID);
	}

	// 공격자가 플레이어면 히트 확정 카메라 셰이크(AI는 PC 없어 자동 스킵).
	// PlaySpace=UserDefined + 공격 방향(Direction)으로 정렬 → 방향성 정도는 에셋이 결정.
	if (CurrentHitEffect.HitCameraShake)
	{
		if (APlayerController* PC = Owner->GetController<APlayerController>())
		{
			PC->ClientStartCameraShake(CurrentHitEffect.HitCameraShake, CurrentHitEffect.HitCameraShakeScale,
			                           ECameraShakePlaySpace::UserDefined, Direction.Rotation());
		}
	}
}

void UACActionInstance::StopInternal(bool bNeedAnimStop)
{
	if (IsPlaying() == false)
	{
		return;
	}

	const bool bBlendingOut = State == EACActionInstanceState::BlendingOut;
	SetState(EACActionInstanceState::Finished);

	if (OwnerComponent)
	{
		OwnerComponent->NotifyActionInstanceStopped(this);
	}

	SetMovementLocked(false);
	EndHitDetection();

	if (AnimInstance == nullptr)
	{
		return;
	}

	// 블렌드아웃 콜백만 끊는다(Ended는 컴포넌트 정리에 필요해 유지).
	if (UACMontageInstanceController* Controller = GetMontageInstanceController())
	{
		Controller->OnBlendingOutStarted.RemoveAll(this);
	}

	FAnimMontageInstance* MontageInstance = AnimInstance->GetMontageInstanceForID(MontageInstanceID);
	if (MontageInstance == nullptr)
	{
		return;
	}

	if (bNeedAnimStop && MontageInstance->IsPlaying() && bBlendingOut == false)
	{
		MontageInstance->Stop(MontageInstance->Montage->BlendOut);
	}
}

void UACActionInstance::SetState(EACActionInstanceState NewState)
{
	if (State == NewState)
	{
		return;
	}

	const EACActionInstanceState PreviousState = State;
	State = NewState;

	UE_LOG(LogActionCore, Verbose, TEXT("[%s] 액션 인스턴스 상태 전환: %s -> %s (%s)"),
	       *GetName(), *UACEnumLibrary::EnumToString(PreviousState), *UACEnumLibrary::EnumToString(NewState),
	       *GetNameSafe(DataAsset ? DataAsset->AnimMontage : nullptr));
}

void UACActionInstance::SetMovementLocked(bool bLock)
{
	if (bMovementLocked == bLock)
	{
		return;
	}

	bMovementLocked = bLock;
	if (CharacterMovementComponent)
	{
		CharacterMovementComponent->SetMovementLocked(bMovementLocked);
	}
}

UACMontageInstanceController* UACActionInstance::GetMontageInstanceController() const
{
	return AnimInstance ? AnimInstance->FindMontageInstanceController(MontageInstanceID) : nullptr;
}

FRotator UACActionInstance::DetermineFacingRotation(const FRotator& InputRotation) const
{
	switch (DataAsset->RotationDirection)
	{
	case EACActionDirection::Input:
		return InputRotation;
	case EACActionDirection::Control:
		return FRotator(0.f, Owner->GetControlRotation().Yaw, 0.f);
	default:
		return Owner->GetActorRotation();
	}
}

bool UACActionInstance::TryLungeWarp(const FRotator& InRotation, FVector& OutLocation, FRotator& OutRotation) const
{
	if (DataAsset->bAutoTargetLunge == false)
	{
		return false;
	}

	AActor* Target = UACTargetingLibrary::FindBestTarget(
		Owner, DataAsset->LungeRange, DataAsset->LungeHalfAngle, InRotation.Vector());
	if (Target == nullptr)
	{
		return false;
	}

	const FVector TargetLocation = Target->GetActorLocation();
	const FVector OwnerLocation = Owner->GetActorLocation();

	const FVector2D Distance2D = FVector2D(TargetLocation - OwnerLocation);
	const float Length = Distance2D.Size();
	if (Length <= KINDA_SMALL_NUMBER)
	{
		return false;
	}
	const FVector ToTargetDirection = FVector(Distance2D, 0.f) / Length;

	// 정지 거리 = 시전자 캡슐R + 타겟 캡슐R + 오프셋.
	const float CasterRadius = Owner->GetCapsuleComponent() ? Owner->GetCapsuleComponent()->GetScaledCapsuleRadius() : 0.f;
	float TargetRadius = 0.f;
	if (const ACharacter* TargetChar = Cast<ACharacter>(Target))
	{
		TargetRadius = TargetChar->GetCapsuleComponent()->GetScaledCapsuleRadius();
	}
	
	const float Standoff = CasterRadius + TargetRadius + DataAsset->LungeOffset;

	// 이미 standoff 안이면 뒤로 끌리지 않게 회전만.
	OutLocation = (Length <= Standoff) ? OwnerLocation : TargetLocation - ToTargetDirection * Standoff;
	OutLocation.Z = OwnerLocation.Z; //에디터에서는 모션워핑의 Z값은 무시되도록 설정되어 있습니다.
	OutRotation = ToTargetDirection.Rotation();
	return true;
}

void UACActionInstance::OnMontageBlendingOutStarted(UAnimMontage* AnimMontage, bool bInterrupted)
{
	if (bInterrupted == false)
	{
		SetState(EACActionInstanceState::BlendingOut);
		MarkMoveCancelable();
		SetActionCancelable(true);
		if (OwnerComponent)
		{
			OwnerComponent->NotifyActionInstanceBlendingOut(this);
		}
		return;
	}

	StopInternal(false);
}

void UACActionInstance::OnMontageEnded(UAnimMontage* AnimMontage, bool bInterrupted)
{
	StopInternal(false);

	if (OwnerComponent)
	{
		OwnerComponent->NotifyActionInstanceEnded(this);
	}
}
