using UnrealBuildTool;
using System.Collections.Generic;

public class PROJECT_LUXTarget : TargetRules
{
	public PROJECT_LUXTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("PROJECT_LUX");
	}
}
