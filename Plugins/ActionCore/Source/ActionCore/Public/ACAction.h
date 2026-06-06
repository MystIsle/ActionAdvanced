// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ACAction.generated.h"

class UACActionInstance;
class UACActionComponent;
class UACActionDataAsset;

UCLASS()
class ACTIONCORE_API UACAction : public UObject
{
	GENERATED_BODY()

public:
	UACAction();
	
	void Initialize(UACActionComponent* InComponent, UACActionDataAsset* InDataAsset);
	UACActionInstance* Play(const FRotator& InRotation);
	bool CanPlay() const;
	
	FGameplayTag GetKey() const { return Key; }
	UACActionDataAsset* GetDataAsset() const { return DataAsset; }
	UACActionComponent* GetOwnerComponent() const { return OwnerComponent; }

private:
	UPROPERTY(VisibleInstanceOnly, Transient)
	FGameplayTag Key;
	
	UPROPERTY(Transient)
	TObjectPtr<UACActionComponent> OwnerComponent;
	
	UPROPERTY(Transient)
	TObjectPtr<UACActionDataAsset> DataAsset;
};
