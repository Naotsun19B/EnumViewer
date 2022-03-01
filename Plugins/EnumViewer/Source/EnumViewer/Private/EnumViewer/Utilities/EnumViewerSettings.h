// Copyright 2022 Naotsun. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "EnumViewerSettings.generated.h"

/**
 * The developer folder view modes used in SEnumViewer.
 */
UENUM()
enum class EEnumViewerDeveloperType : uint8
{
	// Display no developer folders
	None,
	// Allow the current user's developer folder to be displayed.
	CurrentUser,
	// Allow all users' developer folders to be displayed.
	All,
	// Max developer type.
	Max,
};

/**
 * User editor settings for each project for enum viewer.
 */
UCLASS(Config = EditorPerProjectUserSettings)
class ENUMVIEWER_API UEnumViewerSettings : public UObject
{
	GENERATED_BODY()

public:
	// Whether to display internal use structs.
	UPROPERTY(Config)
	bool bDisplayInternalEnums;

	// The developer folder view modes used in SStructViewer
	UPROPERTY(Config)
	EEnumViewerDeveloperType DeveloperFolderType;

public:
	// Returns an event delegate that is executed when a setting has changed.
	DECLARE_EVENT(UEnumViewerSettings, FSettingChangedEvent);
	static FSettingChangedEvent& OnSettingChanged();

private:
	// Holds an event delegate that is executed when a setting has changed.
	static FSettingChangedEvent SettingChangedEvent;
	
public:
	// Returns reference of this settings.
	static const UEnumViewerSettings& Get();

protected:
	// UObject interface.
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	// End of UObject interface.
};

// Partial friend modifier for changing some properties from the SEnumViewer.
namespace EnumViewer
{
	class FEnumViewerSettingsModifier
	{
	private:
		friend class SEnumViewer;
		
		static void SetDisplayInternalEnums(bool bNewState);
		static void SetDeveloperFolderType(EEnumViewerDeveloperType NewType);
	};
}
