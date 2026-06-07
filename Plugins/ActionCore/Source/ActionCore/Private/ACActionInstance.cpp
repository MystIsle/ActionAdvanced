// Fill out your copyright notice in the Description page of Project Settings.


#include "ACActionInstance.h"

#include "ACAction.h"
#include "ACActionComponent.h"
#include "ACActionDataAsset.h"
#include "ACCharacterMovementComponent.h"
#include "ACEnumLibrary.h"
#include "ActionCore.h"
#include "MotionWarpingComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"

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

	AnimInstance = Mesh->GetAnimInstance();
	if (ensure(AnimInstance) == false)
	{
		return;
	}

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

	//NOTE: 런타임 메시-AnimBP 교체시, 콜백 호출 안될 수 있음.
	MontageInstance->OnMontageBlendingOutStarted.BindUObject(this, &ThisClass::OnMontageBlendingOutStarted);
	MontageInstance->OnMontageEnded.BindUObject(this, &ThisClass::OnMontageEnded);

	
	FRotator WarpRotation = ResolveFacingRotation(InRotation);
	
	if (const auto MotionWarpingComp = Owner->GetComponentByClass<UMotionWarpingComponent>())
	{
		static const FName WarpTargetName("Target");
		MotionWarpingComp->AddOrUpdateWarpTargetFromLocationAndRotation(WarpTargetName,
		                                                                Owner->GetActorLocation(),
		                                                                WarpRotation
		);
	}
	else
	{
		UE_LOG(LogActionCore, Warning, TEXT("[%s] 모션워핑 컴포넌트가 존재하지 않습니다. (Owner: %s, Montage: %s)"),
		       *GetName(), *GetNameSafe(Owner), *GetNameSafe(DataAsset->AnimMontage));
	}

	if (CharacterMovementComponent->IsFalling())
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

bool UACActionInstance::IsCancelable() const
{
	return bCancelable;
}

void UACActionInstance::MarkCancelable()
{
	if (bCancelable)
	{
		return;
	}

	bCancelable = true;

	//NOTE: 필요시 이동 캔슬 - 공격 캔슬을 분리해야 한다. 현재는 합쳐져 있는 상태
	SetMovementLocked(false);
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

	if (AnimInstance == nullptr)
	{
		return;
	}

	FAnimMontageInstance* MontageInstance = AnimInstance->GetMontageInstanceForID(MontageInstanceID);
	if (MontageInstance == nullptr)
	{
		return;
	}

	MontageInstance->OnMontageBlendingOutStarted.Unbind();
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

FRotator UACActionInstance::ResolveFacingRotation(const FRotator& InputRotation) const
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

void UACActionInstance::OnMontageBlendingOutStarted(UAnimMontage* AnimMontage, bool bInterrupted)
{
	if (bInterrupted == false)
	{
		SetState(EACActionInstanceState::BlendingOut);
		MarkCancelable();
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
