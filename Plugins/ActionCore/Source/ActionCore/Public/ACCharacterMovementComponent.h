// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ACCharacterMovementComponent.generated.h"

UCLASS()
class ACTIONCORE_API UACCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	virtual FVector ConsumeInputVector() override;
	virtual FRotator GetDeltaRotation(float DeltaTime) const override;

	void SetMovementLocked(bool bLock);
	bool IsMovementInputLocked() const { return bMovementInputLocked; }

protected:
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Rotation")
	float AirRotationScale = 0.1f;

private:
	// 여러 액션이 겹쳐 거는 이동 잠금을 참조 카운트로 관리한다.
	int32 MovementLockedCount = 0;
	bool bMovementInputLocked = false;
	float CurrentRotationScale = 1.0f;
};
