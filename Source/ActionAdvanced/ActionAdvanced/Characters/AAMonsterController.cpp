// Fill out your copyright notice in the Description page of Project Settings.


#include "AAMonsterController.h"


// Sets default values
AAAMonsterController::AAAMonsterController()
{
	PrimaryActorTick.bCanEverTick = true;

}

FGenericTeamId AAAMonsterController::GetGenericTeamId() const
{
	if (const IGenericTeamAgentInterface* TeamPawn = Cast<IGenericTeamAgentInterface>(GetPawn()))
	{
		return TeamPawn->GetGenericTeamId();
	}
	return FGenericTeamId::NoTeam;
}

