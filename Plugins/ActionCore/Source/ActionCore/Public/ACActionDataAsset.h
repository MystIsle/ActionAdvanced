// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "ACActionDataAsset.generated.h"


// 액션 시작 시 캐릭터가 향할 회전 방향의 소스.
UENUM()
enum class EACActionDirection : uint8
{
	None,     // 회전 없음 (현재 액터 방향 유지)
	Input,    // 이동 입력 방향 (기본값)
	Control,  // 카메라/컨트롤 방향 (yaw)
};


UCLASS()
class ACTIONCORE_API UACActionDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
	
public:
	UPROPERTY(EditAnywhere)
	FGameplayTag Key;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<UAnimMontage> AnimMontage;

	// 액션 시작 시 회전 방향 소스. Input은 컨트롤러가 넘긴 입력 방향을 사용한다.
	UPROPERTY(EditAnywhere)
	EACActionDirection RotationDirection = EACActionDirection::Input;
};
