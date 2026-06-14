// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AnimNotifyState_TimedNiagaraEffect.h"
#include "ACAnimNotifyState_TimedNiagara.generated.h"

/**
 * 엔진 Advanced Timed Niagara Effect + 몽타주 인스턴스 컨트롤러 등록(히트스톱 타임스케일 연동).
 * 몽타주 외 재생/ABP 미적용이면 등록 없이 스폰만(엔진 노티파이 스테이트와 동일 동작).
 */
UCLASS(meta = (DisplayName = "Advanced Timed Niagara Effect (ActionCore)"))
class ACTIONCORE_API UACAnimNotifyState_TimedNiagara : public UAnimNotifyState_TimedNiagaraEffectAdvanced
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration,
	                         const FAnimNotifyEventReference& EventReference) override;

	// 메시 애님 타임스케일(히트스톱)을 따라갈지.
	UPROPERTY(EditAnywhere, Category = "ActionCore")
	bool bFollowAnimTimeScale = true;
};
