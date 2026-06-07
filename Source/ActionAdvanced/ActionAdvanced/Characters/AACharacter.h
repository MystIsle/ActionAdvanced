// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MotionWarpingComponent.h"
#include "GameFramework/Character.h"
#include "GenericTeamAgentInterface.h"
#include "Team/AATeamID.h"
#include "AACharacter.generated.h"

class UACActionComponent;

UCLASS()
class ACTIONADVANCED_API AAACharacter : public ACharacter, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	AAACharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UACActionComponent* GetActionComponent() const { return ActionComponent; }
	UMotionWarpingComponent* GetMotionWarpingComponent() const { return MotionWarpingComponent; }

	EAATeamID GetTeamID() const { return TeamID; }

	//~ IGenericTeamAgentInterface
	virtual FGenericTeamId GetGenericTeamId() const override { return static_cast<uint8>(TeamID); }
	//~ End IGenericTeamAgentInterface

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UACActionComponent> ActionComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UMotionWarpingComponent> MotionWarpingComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Team")
	EAATeamID TeamID = EAATeamID::NoTeam;
};
