// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/ReflectedTypeAccessors.h"
#include "ACEnumLibrary.generated.h"


UCLASS()
class ACTIONCORE_API UACEnumLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// UENUM 값을 접두사 없는 짧은 이름 문자열로 변환한다.
	// 예: EACActionInstanceState::Playing -> "Playing" 
	template <typename TEnum>
	static FString EnumToString(TEnum Value)
	{
		static_assert(TIsEnum<TEnum>::Value, "EnumToString는 enum 타입만 지원합니다.");
		return StaticEnum<TEnum>()->GetAuthoredNameStringByValue(static_cast<int64>(Value));
	}
};
