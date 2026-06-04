// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "ACActionDataAsset.generated.h"


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
};
