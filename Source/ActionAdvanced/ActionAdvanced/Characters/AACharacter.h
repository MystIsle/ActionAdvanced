// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AACharacter.generated.h"

class UACActionComponent;

UCLASS()
class ACTIONADVANCED_API AAACharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AAACharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UACActionComponent* GetActionComponent() const { return ActionComponent; }
	
protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UACActionComponent> ActionComponent;
};
