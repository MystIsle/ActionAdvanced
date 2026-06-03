// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AACharacterMovementComponent.generated.h"

UCLASS()
class ACTIONADVANCED_API UAACharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	virtual FRotator GetDeltaRotation(float DeltaTime) const override;

protected:
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Rotation")
	float AirRotationScale = 0.3f;

private:
	// OnMovementModeChanged에서 모드 전환 시 한 번만 갱신되는 회전 스케일 캐시
	float CurrentRotationScale = 1.0f;
};
