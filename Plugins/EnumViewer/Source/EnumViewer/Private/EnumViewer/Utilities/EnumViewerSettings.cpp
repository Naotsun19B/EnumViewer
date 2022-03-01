// Copyright 2022 Naotsun. All Rights Reserved.

#include "EnumViewer/Utilities/EnumViewerSettings.h"
#include "UnrealEdMisc.h"

#define LOCTEXT_NAMESPACE "EnumViewerSettings"

UEnumViewerSettings::FSettingChangedEvent UEnumViewerSettings::SettingChangedEvent;

UEnumViewerSettings::FSettingChangedEvent& UEnumViewerSettings::OnSettingChanged()
{
	return SettingChangedEvent;
}

const UEnumViewerSettings& UEnumViewerSettings::Get()
{
	const auto* Settings = GetDefault<UEnumViewerSettings>();
	check(IsValid(Settings));
	return *Settings;
}

void UEnumViewerSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property == nullptr)
	{
		return;
	}
	
	if (!FUnrealEdMisc::Get().IsDeletePreferences())
	{
		SaveConfig();
	}

	if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UEnumViewerSettings, bDisplayInternalEnums) ||
		PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UEnumViewerSettings, DeveloperFolderType))
	{
		SettingChangedEvent.Broadcast();
	}
}

namespace EnumViewer
{
	void FEnumViewerSettingsModifier::SetDisplayInternalEnums(bool bNewState)
	{
		if (auto* Settings = GetMutableDefault<UEnumViewerSettings>())
		{
			Settings->bDisplayInternalEnums = bNewState;
			Settings->PostEditChange();
		}
	}

	void FEnumViewerSettingsModifier::SetDeveloperFolderType(EEnumViewerDeveloperType NewType)
	{
		if (auto* Settings = GetMutableDefault<UEnumViewerSettings>())
		{
			Settings->DeveloperFolderType = NewType;
			Settings->PostEditChange();
		}
	}
}

#undef LOCTEXT_NAMESPACE
