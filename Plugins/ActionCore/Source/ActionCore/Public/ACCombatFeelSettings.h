// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "ACCombatFeelSettings.generated.h"

class UMaterialInterface;
class UCurveFloat;

// 타격감 연출 전역 설정. Project Settings > Game > "Action Core - Combat Feel".
UCLASS(config = Game, defaultconfig, meta = (DisplayName = "Action Core - Combat Feel"))
class ACTIONCORE_API UACCombatFeelSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override { return FName(TEXT("Game")); }

	// 히트 점멸 마스터 스위치.
	UPROPERTY(EditAnywhere, config, Category = "Hit Flash")
	bool bEnableHitFlash = true;

	// 오버레이용 점멸 머터리얼(Unlit + Additive, FlashColor/FlashAmount 파라미터). 미지정이면 점멸 없음.
	UPROPERTY(EditAnywhere, config, Category = "Hit Flash", meta = (AllowedClasses = "/Script/Engine.MaterialInterface"))
	TSoftObjectPtr<UMaterialInterface> FlashMaterial;

	// 점멸 색(Emissive 입력).
	UPROPERTY(EditAnywhere, config, Category = "Hit Flash")
	FLinearColor FlashColor = FLinearColor::White;

	// 피크 강도(MID FlashAmount = Alpha * FlashIntensity).
	UPROPERTY(EditAnywhere, config, Category = "Hit Flash", meta = (ClampMin = "0.0"))
	float FlashIntensity = 1.0f;

	// 감쇠 길이(s). 고정. 히트스톱과 비동기화(실시간).
	UPROPERTY(EditAnywhere, config, Category = "Hit Flash", meta = (ClampMin = "0.0"))
	float FlashDuration = 0.1f;

	// 감쇠 곡선(X=0~1 정규화 시간, Y=0~1 강도). null이면 선형(1-t) falloff.
	UPROPERTY(EditAnywhere, config, Category = "Hit Flash")
	TSoftObjectPtr<UCurveFloat> FlashCurve;
};
