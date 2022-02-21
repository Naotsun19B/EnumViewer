// Copyright 2022 Naotsun. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class IPropertyHandle;

namespace EnumViewer
{
	class IEnumViewerFilter;
	
	/**
	 * The mode of the viewer to start.
	 */
	enum class EEnumViewerMode : uint8
	{
		// Allows all enums to be browsed and selected; syncs selection with the editor; drag and drop attachment, etc.
		EnumBrowsing,

		// Sets the enum viewer to operate as a Enum 'picker'.
		EnumPicker,
	};

	/**
	 *  The type of enum notation displayed in the viewer.
	 */
	enum class EEnumViewerNameTypeToDisplay : uint8
	{
		// Display both the display name and enum name if they're available and different.
		Dynamic,

		// Always use the display name
		DisplayName,

		// Always use the Enum name
		EnumName,
	};

	/**
	 * Settings for the enum viewer set by the programmer before spawning an instance of the widget.  
	 * This is used to modify the enum viewer's behavior in various ways, such as filtering in or out specific enums.
	 */
	class FEnumViewerInitializationOptions
	{
	public:
		// The filter to use on enums in this instance.
		TSharedPtr<IEnumViewerFilter> EnumFilter;

		// Mode to operate in.
		EEnumViewerMode Mode;
		
		// Shows unloaded enums. Will not be filtered out based on non-bool filter options.
		bool bShowUnloadedEnums;

		// Shows a "None" option, only available in picker mode.
		bool bShowNoneOption;

		// If true, root nodes will be expanded by default.
		bool bExpandRootNodes;

		// If true, allows enum dynamic loading on selection.
		bool bEnableEnumDynamicLoading;

		// Controls what name is shown for enums.
		EEnumViewerNameTypeToDisplay NameTypeToDisplay;

		// The title string of the enum viewer if required.
		FText ViewerTitleString;

		// The property this enum viewer be working on.
		TSharedPtr<IPropertyHandle> PropertyHandle;

		// If true (the default), shows the view options at the bottom of the enum picker.
		bool bAllowViewOptions;

		// If true (the default), shows a background border behind the enum viewer widget.
		bool bShowBackgroundBorder;

		// Defines additional enums you want listed in the "Common Enums" section for the picker.
		TArray<const UEnum*> ExtraPickerCommonEnums;

	public:
		// Constructor.
		FEnumViewerInitializationOptions()
			: Mode(EEnumViewerMode::EnumPicker)
			, bShowUnloadedEnums(true)
			, bShowNoneOption(false)
			, bExpandRootNodes(true)
			, bEnableEnumDynamicLoading(true)
			, NameTypeToDisplay(EEnumViewerNameTypeToDisplay::EnumName)
			, bAllowViewOptions(true)
			, bShowBackgroundBorder(true)
		{
		}
	};
}
