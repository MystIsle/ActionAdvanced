// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AACharacter.generated.h"

UCLASS()
class ACTIONADVANCED_API AAACharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AAACharacter();

public:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
};
