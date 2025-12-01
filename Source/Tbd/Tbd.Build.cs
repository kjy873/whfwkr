// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class Tbd : ModuleRules
{
	public Tbd(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "Sockets", "Networking","AIModule", "GameplayTasks", "NavigationSystem",
															"Landscape", "Foliage", "GameplayTags", "EnhancedInput"});

		PrivateDependencyModuleNames.AddRange(new string[] { "ProtobufCore" });

		PrivateIncludePaths.AddRange(new string[] { 
			"Tbd/",
			"Tbd/Network/",
			"Tbd/Game/",
			"Tbd/Player/",
        });

        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.Add("LandscapeEditor");
        }

        PrivateDependencyModuleNames.AddRange(new string[] { });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
