// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "GenericTeamAgentInterface.h"
#include "AAMonsterController.generated.h"

UCLASS()
class ACTIONADVANCED_API AAAMonsterController : public AAIController
{
	GENERATED_BODY()

public:
	AAAMonsterController();

	virtual FGenericTeamId GetGenericTeamId() const override;
};
