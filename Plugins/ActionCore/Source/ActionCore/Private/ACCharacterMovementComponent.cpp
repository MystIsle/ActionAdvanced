// Fill out your copyright notice in the Description page of Project Settings.

#include "ACCharacterMovementComponent.h"

void UACCharacterMovementComponent::SetMovementLocked(bool bLock)
{
	// 겹쳐 거는 락을 카운트로 관리하고, 0<->1 경계에서만 실제 입력 잠금을 토글한다.
	const bool bPrevLocked = MovementLockedCount > 0;
	MovementLockedCount += bLock ? 1 : -1;
	const bool bCurrentLocked = MovementLockedCount > 0;

	if (bPrevLocked == bCurrentLocked)
	{
		return;
	}

	bMovementInputLocked = bCurrentLocked;
}

FVector UACCharacterMovementComponent::ConsumeInputVector()
{
	//TODO : 길찾기 : RequestDirectMove(RequestedVelocity 경로)는 ConsumeInputVector를 안 거치므로 이걸로 안 막힘. 별도 대응 필요.
	
	const FVector Input = Super::ConsumeInputVector();
	return bMovementInputLocked ? FVector::ZeroVector : Input;
}

FRotator UACCharacterMovementComponent::GetDeltaRotation(float DeltaTime) const
{
	return Super::GetDeltaRotation(DeltaTime) * CurrentRotationScale;
}

void UACCharacterMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);

	CurrentRotationScale = IsFalling() ? AirRotationScale : 1.0f;
}
