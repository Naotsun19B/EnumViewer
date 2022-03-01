// Copyright 2022 Naotsun. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UObject/SoftObjectPtr.h"
#include "Engine/EngineTypes.h"
#include "EnumViewerProjectSettings.generated.h"

/**
 * Project settings for enum viewer.
 */
UCLASS(Config = Engine, defaultconfig)
class ENUMVIEWER_API UEnumViewerProjectSettings : public UObject
{
	GENERATED_BODY()

public:
	// The base directories to be considered Internal Only for the enum picker.
	UPROPERTY(EditAnywhere, Config, Category = "Enum Visibility Management", meta = (DisplayName = "List of directories to consider Internal Only.", ContentDir, LongPackageName))
	TArray<FDirectoryPath> InternalOnlyPaths;

	// The base classes to be considered Internal Only for the enum picker.
	UPROPERTY(EditAnywhere, Config, Category = "Enum Visibility Management", meta = (DisplayName = "List of base enums to consider Internal Only.", ShowTreeView, HideViewOptions))
	TArray<TSoftObjectPtr<const UEnum>> InternalOnlyEnums;

public:
	// Constructor.
	UEnumViewerProjectSettings();
	
	// Register - unregister in the editor setting item.
	static void Register();
	static void Unregister();
	
	// Returns reference of this settings.
	static const UEnumViewerProjectSettings& Get();
};
