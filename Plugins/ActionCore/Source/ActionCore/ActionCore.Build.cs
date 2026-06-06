// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ActionCore : ModuleRules
{
	public ActionCore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;


		PublicDependencyModuleNames.AddRange(
			[
				"Core",
				"CoreUObject",
				"Engine",
				"GameplayTags"
			]
		);

		PrivateDependencyModuleNames.AddRange(["MotionWarping"]);

		PrivateIncludePaths.AddRange([ModuleDirectory]);
	}
}