// Fill out your copyright notice in the Description page of Project Settings.

#include "ACHitReactionComponent.h"

#include "ACActionComponent.h"
#include "ACActionInstance.h"
#include "ACCharacterMovementComponent.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/RootMotionSource.h"
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
	// 리액션 도중 파괴/레벨전환/빙의교체 시 락·애님레이트가 재사용 CMC/메시에 남지 않도록 정리.
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

	Super::EndPlay(EndPlayReason);
}

void UACHitReactionComponent::PlayReact(const FACHitEffect& Effect, const FVector& Direction)
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

	// 3. 피격 몽타주 (배열 순차 재생, 끝나면 0으로 순환)
	if (HitReactMontages.Num() > 0)
	{
		const int32 Index = HitMontageIndex % HitReactMontages.Num();
		HitMontageIndex = (HitMontageIndex + 1) % HitReactMontages.Num();
		if (UAnimMontage* Montage = HitReactMontages[Index])
		{
			OwnerCharacter->PlayAnimMontage(Montage);
		}
	}

	// 넉백 파라미터 보관(히트스탑 후 StartKnockback에서 사용)
	PendingKnockbackDir = Direction.GetSafeNormal2D();
	PendingKnockbackDistance = Effect.KnockbackDistance;
	PendingKnockbackDuration = Effect.KnockbackDuration;
	PendingKnockbackCurve = Effect.KnockbackEaseCurve;

	// 4. 히트스탑(단일 진입점) — 애님 멈춤, 자체 복원
	RequestHitStop(Effect.HitStopPlayRate, Effect.HitStopDuration);

	// 5. 넉백 타이밍: 히트스탑 후
	if (Effect.HitStopDuration > 0.f)
	{
		OwnerCharacter->GetWorldTimerManager().SetTimer(
			ReactTimer, this, &UACHitReactionComponent::StartKnockback, Effect.HitStopDuration, false);
	}
	else
	{
		StartKnockback();
	}
}

void UACHitReactionComponent::RequestHitStop(float Scale, float Duration)
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
