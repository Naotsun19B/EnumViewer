// Copyright 2022 Naotsun. All Rights Reserved.

#include "EnumViewerSettings.h"
#include "Modules/ModuleManager.h"
#include "ISettingsModule.h"
#include "UnrealEdMisc.h"

#define LOCTEXT_NAMESPACE "EnumViewerSettings"

namespace EnumViewer
{
	namespace Settings
	{
		static const FName ContainerName    = TEXT("Editor");
		static const FName CategoryName	    = TEXT("Plugins");
		static const FName SectionName      = TEXT("EnumViewerSettings");

		ISettingsModule* GetSettingsModule()
		{
			return FModuleManager::GetModulePtr<ISettingsModule>("Settings");
		}
	}
}

UEnumViewerSettings::UEnumViewerSettings()
	: Super()
{
}

void UEnumViewerSettings::Register()
{
	if (ISettingsModule* SettingsModule = EnumViewer::Settings::GetSettingsModule())
	{
		SettingsModule->RegisterSettings(
			EnumViewer::Settings::ContainerName,
			EnumViewer::Settings::CategoryName,
			EnumViewer::Settings::SectionName,
			LOCTEXT("SettingName", "Enum Viewer"),
			LOCTEXT("SettingDescription", "Editor settings for Enum Viewer"),
			GetMutableDefault<UEnumViewerSettings>()
		);
	}
}

void UEnumViewerSettings::Unregister()
{
	if (ISettingsModule* SettingsModule = EnumViewer::Settings::GetSettingsModule())
	{
		SettingsModule->UnregisterSettings(
			EnumViewer::Settings::ContainerName,
			EnumViewer::Settings::CategoryName,
			EnumViewer::Settings::SectionName
		);
	}
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

	SettingChangedEvent.Broadcast(PropertyChangedEvent.Property->GetFName());
}

#undef LOCTEXT_NAMESPACE
