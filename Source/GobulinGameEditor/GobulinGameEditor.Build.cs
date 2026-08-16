using UnrealBuildTool;

public class GobulinGameEditor : ModuleRules
{
	public GobulinGameEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"UnrealEd",
			"AssetTools",
			"EnhancedInput",
			"GobulinGame"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });
	}
}
