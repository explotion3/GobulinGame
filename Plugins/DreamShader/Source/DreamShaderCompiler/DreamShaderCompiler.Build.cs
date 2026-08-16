using UnrealBuildTool;

public class DreamShaderCompiler : ModuleRules
{
	public DreamShaderCompiler(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new[]
			{
				"Core",
				"CoreUObject",
				"DreamShader",
				"Engine"
			});
	}
}
