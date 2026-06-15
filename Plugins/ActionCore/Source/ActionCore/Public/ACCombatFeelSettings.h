// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "ACCombatFeelSettings.generated.h"

class UMaterialInterface;

// 연출 레이어 — 런타임 on/off 토글(AA.Feel.* CVar) 대상. 강의 데모 누적 시연용.
enum class EACFeelLayer : uint8
{
	HitStop,
	HitFX,
	Flash,
	CameraShake,
	MeshShake,
};

// 타격감 연출 전역 설정. Project Settings > Game > "Action Core - Combat Feel".
UCLASS(config = Game, defaultconfig, meta = (DisplayName = "Action Core - Combat Feel"))
class ACTIONCORE_API UACCombatFeelSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override { return FName(TEXT("Game")); }

	// 연출 레이어 런타임 게이트: AA.Feel.Master && AA.Feel.<Layer>. 데모 토글(CVar)용.
	static bool IsFeelLayerEnabled(EACFeelLayer Layer);

	// 데모 토글 입력/오버레이 지원 (게임 모듈의 키 바인딩·UI가 호출).
	static void ToggleFeelLayer(EACFeelLayer Layer);
	static void ToggleFeelMaster();
	static bool GetFeelLayerRaw(EACFeelLayer Layer);   // 개별 CVar 값(마스터 무관)
	static bool GetFeelMaster();
	static const TCHAR* GetFeelLayerLabel(EACFeelLayer Layer);

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

	// 감쇠 지수. falloff = pow(1-t, Exponent). 클수록 빠르게 꺼짐(1=선형).
	UPROPERTY(EditAnywhere, config, Category = "Hit Flash", meta = (ClampMin = "0.0"))
	float FlashExponent = 2.0f;
};
