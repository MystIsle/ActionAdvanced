// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MotionWarpingComponent.h"
#include "MeleeTraceComponent.h"
#include "GameFramework/Character.h"
#include "GenericTeamAgentInterface.h"
#include "Team/AATeamID.h"
#include "ACHittable.h"
#include "AACharacter.generated.h"

class UACActionComponent;
class UMeleeTraceComponent;
class UACHitReactionComponent;

UCLASS()
class ACTIONADVANCED_API AAACharacter : public ACharacter, public IGenericTeamAgentInterface, public IACHittable
{
	GENERATED_BODY()

public:
	AAACharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UACActionComponent* GetActionComponent() const { return ActionComponent; }
	UMotionWarpingComponent* GetMotionWarpingComponent() const { return MotionWarpingComponent; }
	UMeleeTraceComponent* GetMeleeTraceComponent() const { return MeleeTraceComponent; }

	EAATeamID GetTeamID() const { return TeamID; }

	virtual FGenericTeamId GetGenericTeamId() const override { return static_cast<uint8>(TeamID); }

	virtual void ReceiveHit(const FACHitInfo& HitInfo) override;

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UACActionComponent> ActionComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UMotionWarpingComponent> MotionWarpingComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UMeleeTraceComponent> MeleeTraceComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UACHitReactionComponent> HitReactionComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Team")
	EAATeamID TeamID = EAATeamID::NoTeam;
};
