// Copyright 2022 Naotsun. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UUserDefinedEnum;

namespace EnumViewer
{
	class FEnumViewerInitializationOptions;

	/**
	 * A class that defines utility functions related to enums used in enum viewers.
	 */
	class ENUMVIEWER_API FEnumViewerUtils
	{
	public:
		// Gets the display name specified by the target UENUM.
		static FText GetEnumDisplayName(const TWeakObjectPtr<const UEnum> InEnum);

		// Checks if the enum is allowed under the init options of the enum viewer currently building it's list.
		static bool IsEnumAllowed(
			const FEnumViewerInitializationOptions& InInitOptions,
			const TWeakObjectPtr<const UEnum> InEnum
		);

		// Checks if the unloaded enum is allowed under the init options of the enum viewer currently building it's list.
		static bool IsUnloadedEnumAllowed(
			const FEnumViewerInitializationOptions& InInitOptions,
			const FName InEnumPath
		);

		// Opens a enum source file.
		static void OpenEnumInIDE(const UEnum* InEnum);
		
		// Opens an asset editor for a user defined enum.
		static void OpenAssetEditor(const UUserDefinedEnum* InEnum);

		// Finds the enum in the content browser.
		static void FindInContentBrowser(const UEnum* InEnum);

		// Generates and returns a context menu widget for enums.
		static TSharedRef<SWidget> GenerateContextMenuWidget(const UEnum* InEnum);
	};
}
