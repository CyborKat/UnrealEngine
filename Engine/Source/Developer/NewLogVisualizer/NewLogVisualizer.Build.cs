// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class NewLogVisualizer : ModuleRules
{
	public NewLogVisualizer(TargetInfo Target)
	{
        PrivateIncludePathModuleNames.AddRange(
			new string[] {
				"DesktopPlatform",
				"MainFrame",
                "SequencerWidgets",
			}
		);

		PublicIncludePaths.AddRange(
			new string[] {
				"Developer/TaskGraph/Public",
				"Runtime/Engine/Classes",
                "Editor/GeometryMode/Public"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
                "InputCore",
				"Json",
				"Slate",
				"SlateCore",
                "EditorStyle",
				"Engine",
				"TaskGraph",
                "GeometryMode",
				"UnrealEd",
                "SequencerWidgets",
			}
		);

		DynamicallyLoadedModuleNames.AddRange(
			new string[] {
				"DesktopPlatform",
				"MainFrame",
			}
		);
	}
}
