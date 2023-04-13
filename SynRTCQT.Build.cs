// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class SynRTCQT : ModuleRules
{
	public SynRTCQT(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "AudioSynesthesia" , "AudioSynesthesiaCore", "SignalProcessing"});

		PrivateDependencyModuleNames.AddRange(new string[] {"AudioSynesthesia", "AudioSynesthesiaCore" , "SignalProcessing" });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
