// Fill out your copyright notice in the Description page of Project Settings.


#include "ACActionDataAsset.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"

EDataValidationResult UACActionDataAsset::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	if (Key.IsValid() == false)
	{
		Context.AddError(FText::FromString(TEXT("Key(GameplayTag)가 설정되지 않았습니다.")));
		Result = EDataValidationResult::Invalid;
	}
	
	if (AnimMontage == nullptr)
	{
		Context.AddError(FText::FromString(TEXT("AnimMontage가 비어 있습니다.")));
		Result = EDataValidationResult::Invalid;
	}

	return Result;
}
#endif
