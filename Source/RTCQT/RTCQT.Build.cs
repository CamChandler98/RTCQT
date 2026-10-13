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
			"Engine",
			"UMG",
			"SlateCore"
		});

		PrivateDependencyModuleNames.AddRange(new[]
		{
			"AudioSynesthesia",
			"AudioAnalyzer",
			"AudioSynesthesiaCore"
		});

		// The actor's property setter is engine-only. Real sequence integration
		// fixtures compile only with editor development automation.
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new[] { "LevelSequence", "MovieScene", "MovieSceneTracks" });
		}
	}
}
