using UnrealBuildTool;

/// <summary>Defines the editor-independent dependency boundary for the RTCQT runtime module.</summary>
public class RTCQT : ModuleRules
{
	/// <summary>Configures only the runtime modules required by the D04 public contract.</summary>
	public RTCQT(ReadOnlyTargetRules Target) : base(Target)
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
			"AudioSynesthesia"
		});
	}
}
