// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System;
using System.IO;

public class DS5W_UE4 : ModuleRules
{
	public DS5W_UE4(ReadOnlyTargetRules Target) : base(Target)
	{
		bEnableUndefinedIdentifierWarnings = false;

		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicAdditionalLibraries.Add("C:/Program Files (x86)/Windows Kits/10/Lib/10.0.17134.0/um/x64/hid.lib");
		PublicDefinitions.Add("DS5W_USE_LIB");

		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",      // Provides Actors and Structs
                "Engine",           // Used by Actor
                "Slate",            // Used by InputDevice to fire bespoke FKey events
                "InputCore",        // Provides LOCTEXT and other Input features
                "InputDevice",
				"ApplicationCore"
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
