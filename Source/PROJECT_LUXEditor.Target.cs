using UnrealBuildTool;
using System.Collections.Generic;

public class PROJECT_LUXEditorTarget : TargetRules
{
	public PROJECT_LUXEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("PROJECT_LUX");
	}
}
