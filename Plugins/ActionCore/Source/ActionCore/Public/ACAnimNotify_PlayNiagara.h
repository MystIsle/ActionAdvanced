// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AnimNotify_PlayNiagaraEffect.h"
#include "ACAnimNotify_PlayNiagara.generated.h"

/**
 * 엔진 Play Niagara 노티파이 + 몽타주 인스턴스 컨트롤러 등록(히트스톱 타임스케일 연동).
 * 몽타주 외 재생/ABP 미적용이면 등록 없이 스폰만(엔진 노티파이와 동일 동작).
 */
UCLASS(meta = (DisplayName = "Play Niagara (ActionCore)"))
class ACTIONCORE_API UACAnimNotify_PlayNiagara : public UAnimNotify_PlayNiagaraEffect
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	                    const FAnimNotifyEventReference& EventReference) override;

	// 메시 애님 타임스케일(히트스톱)을 따라갈지.
	UPROPERTY(EditAnywhere, Category = "ActionCore")
	bool bFollowAnimTimeScale = true;
};
