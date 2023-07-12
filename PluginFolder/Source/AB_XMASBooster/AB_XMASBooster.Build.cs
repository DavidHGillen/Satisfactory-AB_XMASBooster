using UnrealBuildTool;
using System.IO;
using System;
using System.Runtime.InteropServices;
using System.Text;
using EpicGames.Core;

public class AB_XMASBooster : ModuleRules
{
	public AB_XMASBooster(ReadOnlyTargetRules Target) : base(Target) {
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
            "Core", "CoreUObject",
            "Engine",
            "DeveloperSettings",
            "PhysicsCore",
            "InputCore",
            "OnlineSubsystem", "OnlineSubsystemNull", "OnlineSubsystemUtils",
            "SignificanceManager",
            "GeometryCollectionEngine",
            "ChaosVehiclesCore", "ChaosVehicles", "ChaosSolverEngine",
            "AnimGraphRuntime",
            "AkAudio",
            "AssetRegistry",
            "NavigationSystem",
            "ReplicationGraph",
            "AIModule",
            "GameplayTasks",
            "SlateCore", "Slate", "UMG",
            "InstancedSplines",
            "RenderCore",
            "CinematicCamera",
            "Foliage",
            "Niagara",
            "EnhancedInput",
            "GameplayCameras",
            "TemplateSequence",
            "NetCore",
            "GameplayTags",
		});


		if (Target.Type == TargetRules.TargetType.Editor) {
			PublicDependencyModuleNames.AddRange(new string[] {"OnlineBlueprintSupport", "AnimGraph"});
		}

		PublicDependencyModuleNames.AddRange(new string[] {"FactoryGame", "SML"});
	}
}