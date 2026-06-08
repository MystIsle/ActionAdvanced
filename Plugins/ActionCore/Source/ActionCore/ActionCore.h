// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/** ActionCore 플러그인 전역 로그 카테고리 */
DECLARE_LOG_CATEGORY_EXTERN(LogActionCore, Verbose, All);

class FActionCoreModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
