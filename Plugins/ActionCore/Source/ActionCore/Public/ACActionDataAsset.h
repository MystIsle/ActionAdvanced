// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "ACActionDataAsset.generated.h"


UENUM()
enum class EACActionDirection : uint8
{
	None,
	Input,
	Control,
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

	UPROPERTY(EditAnywhere)
	EACActionDirection RotationDirection = EACActionDirection::Input;

	UPROPERTY(EditAnywhere, Category = "AutoTarget")
	bool bAutoTargetLunge = false;

	UPROPERTY(EditAnywhere, Category = "AutoTarget", meta = (EditCondition = "bAutoTargetLunge", ClampMin = "0.0"))
	float LungeRange = 400.f;

	UPROPERTY(EditAnywhere, Category = "AutoTarget", meta = (EditCondition = "bAutoTargetLunge", ClampMin = "0.0", ClampMax = "180.0"))
	float LungeHalfAngle = 60.f;

	UPROPERTY(EditAnywhere, Category = "AutoTarget", meta = (EditCondition = "bAutoTargetLunge"))
	float LungeOffset = 0.f;
};
