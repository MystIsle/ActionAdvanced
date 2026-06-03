// Fill out your copyright notice in the Description page of Project Settings.

#include "AACharacterMovementComponent.h"
#include "ActionAdvanced.h"

FRotator UAACharacterMovementComponent::GetDeltaRotation(float DeltaTime) const
{
	return Super::GetDeltaRotation(DeltaTime) * CurrentRotationScale;
}

void UAACharacterMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);

	// 모드 전환 시점에 회전 스케일을 한 번만 결정해 캐시 (GetDeltaRotation에서 매 프레임 분기 제거)
	CurrentRotationScale = IsFalling() ? AirRotationScale : 1.0f;

	if (MovementMode == MOVE_Falling)
	{
		UE_LOG(LogActionAdvanced, Log, TEXT("CMC: Entered Falling"));
	}
	else if (PreviousMovementMode == MOVE_Falling)
	{
		UE_LOG(LogActionAdvanced, Log, TEXT("CMC: Landed (from Falling)"));
	}
}
