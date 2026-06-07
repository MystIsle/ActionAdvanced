// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ActionAdvanced : ModuleRules
{
	public ActionAdvanced(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate",
			"ActionCore",
			"GameplayTags",
			"MotionWarping"
		});

		PrivateDependencyModuleNames.AddRange([]);

		PublicIncludePaths.AddRange(new string[] {
			"ActionAdvanced",
			// 활성 코드 서브디렉터리를 include 루트로 추가해, 폴더 상대 include("Team/AATeamID.h" 등)를 허용한다.
			"ActionAdvanced/ActionAdvanced",
			"ActionAdvanced/Variant_Platforming",
			"ActionAdvanced/Variant_Platforming/Animation",
			"ActionAdvanced/Variant_Combat",
			"ActionAdvanced/Variant_Combat/AI",
			"ActionAdvanced/Variant_Combat/Animation",
			"ActionAdvanced/Variant_Combat/Gameplay",
			"ActionAdvanced/Variant_Combat/Interfaces",
			"ActionAdvanced/Variant_Combat/UI",
			"ActionAdvanced/Variant_SideScrolling",
			"ActionAdvanced/Variant_SideScrolling/AI",
			"ActionAdvanced/Variant_SideScrolling/Gameplay",
			"ActionAdvanced/Variant_SideScrolling/Interfaces",
			"ActionAdvanced/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
