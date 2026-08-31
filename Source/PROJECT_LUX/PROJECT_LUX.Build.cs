using UnrealBuildTool;

public class PROJECT_LUX : ModuleRules
{
	public PROJECT_LUX(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new[]
		{
			"Core",
			"CoreUObject",
			"Engine"
		});

		PrivateDependencyModuleNames.AddRange(new[]
		{
			"EnhancedInput",
			"Niagara",
			"OnlineBase",
			"OnlineSubsystem",
			"OnlineSubsystemUtils"
		});
	}
}
