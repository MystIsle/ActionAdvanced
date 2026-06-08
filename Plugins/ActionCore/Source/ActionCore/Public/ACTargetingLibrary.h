// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ACTargetingLibrary.generated.h"

class ACharacter;

/**
 * 오토타게팅 선택 유틸. 상태 없음(stateless) — 스킬 발동 시 1회 호출해 최적 적 타겟을 고른다.
 * 적/아군 판정은 엔진 IGenericTeamAgentInterface(FGenericTeamId::GetAttitude)로만 — 게임별 팀 enum을 모름.
 * CVar "AA.AutoTarget.Debug 1"이면 고른 타겟/콘을 잠깐 그린다.
 */
UCLASS()
class ACTIONCORE_API UACTargetingLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Source 기준 Range/HalfAngle 콘 안에서 적(Hostile) 중 가중점수 최고 타겟을 반환. 없으면 nullptr.
	// AimDirection: 콘 중심 방향(월드, 내부에서 2D 정규화). RangeWeightRate: 0~1, 거리/각도 가중 블렌드.
	UFUNCTION(BlueprintCallable, Category = "ActionCore|Targeting")
	static AActor* FindBestTarget(ACharacter* Source, float Range, float HalfAngle,
	                                     FVector AimDirection, float RangeWeightRate = 0.5f);
};
