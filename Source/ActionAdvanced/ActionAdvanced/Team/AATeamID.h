// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

// 팀 정체성. AAACharacter가 IGenericTeamAgentInterface로 노출하고
// 컨트롤러가 폰에 위임한다. 적대 판정은 엔진 기본 attitude 솔버 사용.
UENUM(BlueprintType)
enum class EAATeamID : uint8
{
	Player  = 0,
	Monster = 1,
	NoTeam  = 255,
};
