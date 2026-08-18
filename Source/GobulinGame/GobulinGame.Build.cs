// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class GobulinGame : ModuleRules
{
	public GobulinGame(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"LevelSequence",
			"MovieScene",
			"MovieSceneTracks",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"GameplayTags",
			"CommonUI",
			"CommonInput",
			"MassCommon",
			"MassActors",
			"MassSpawner",
			"MassSimulation",
			"MassLOD",
			"MassMovement",
			"MassRepresentation",
			"MassReplication",
			"Niagara",
			"Paper2D",
			"UMG",
			"Slate",
			"SlateCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"NavigationSystem"
		});

		PublicIncludePaths.AddRange(new string[] {
			"GobulinGame",
			"GobulinGame/Core",
			"GobulinGame/Combat",
			"GobulinGame/Horde",
			"GobulinGame/Base",
			"GobulinGame/Meta",
			"GobulinGame/Net",
			"GobulinGame/Data",
			"GobulinGame/UI",
			"GobulinGame/AI",
			"GobulinGame/Variant_Horror",
			"GobulinGame/Variant_Horror/UI",
			"GobulinGame/Variant_Shooter",
			"GobulinGame/Variant_Shooter/AI",
			"GobulinGame/Variant_Shooter/UI",
			"GobulinGame/Variant_Shooter/Weapons"
		});

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
