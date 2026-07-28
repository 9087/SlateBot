// Copyright 2025 Wu Zhiwei. All Rights Reserved.

using UnrealBuildTool;

public class SlateBot : ModuleRules
{
	public SlateBot(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"UMG",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"ApplicationCore",
			"Slate",
			"SlateCore",
		});
	}
}
