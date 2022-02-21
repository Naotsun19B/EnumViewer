// Copyright 2022 Naotsun. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UObject/SoftObjectPtr.h"
#include "Engine/EngineTypes.h"
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
 * Editor settings for enum viewer.
 */
UCLASS(config = Editor, defaultconfig)
class ENUMVIEWER_API UEnumViewerSettings : public UObject
{
	GENERATED_BODY()

public:
	// The base directories to be considered Internal Only for the enum picker.
	UPROPERTY(EditAnywhere, Config, Category = "Enum Visibility Management", meta = (DisplayName = "List of directories to consider Internal Only.", ContentDir, LongPackageName))
	TArray<FDirectoryPath> InternalOnlyPaths;

	// The base classes to be considered Internal Only for the enum picker.
	UPROPERTY(EditAnywhere, Config, Category = "Enum Visibility Management", meta = (DisplayName = "List of base enums to consider Internal Only.", ShowTreeView, HideViewOptions))
	TArray<TSoftObjectPtr<const UEnum>> InternalOnlyEnums;

	// Whether to display internal use structs.
	UPROPERTY(EditAnywhere, Config, Category = "Enum Viewer")
	bool bDisplayInternalEnums;

	// The developer folder view modes used in SStructViewer
	UPROPERTY(EditAnywhere, Config, Category = "Enum Viewer")
	EEnumViewerDeveloperType DeveloperFolderType;

public:
	// Returns an event delegate that is executed when a setting has changed.
	DECLARE_EVENT_OneParam(UEnumViewerSettings, FSettingChangedEvent, FName /* PropertyName */);
	static FSettingChangedEvent& OnSettingChanged() { return SettingChangedEvent; }

private:
	// Holds an event delegate that is executed when a setting has changed.
	static FSettingChangedEvent SettingChangedEvent;
	
public:
	// Constructor.
	UEnumViewerSettings();
	
	// Register - unregister in the editor setting item.
	static void Register();
	static void Unregister();
	
	// Returns reference of this settings.
	static const UEnumViewerSettings& Get();

	// Partial friend modifier for changing some properties from the SEnumViewer.
	struct FEnumViewerModifier
	{
	public:
		friend class SEnumViewer;
		UEnumViewerSettings* This = nullptr;

		FEnumViewerModifier();

		void SetDisplayInternalEnums(bool bNewState);
		void SetDeveloperFolderType(EEnumViewerDeveloperType NewType);
	};
	
protected:
	// UObject interface.
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	// End of UObject interface.
};
