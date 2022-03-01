// Copyright 2022 Naotsun. All Rights Reserved.

#include "EnumViewer/Utilities/EnumViewerProjectSettings.h"
#include "Modules/ModuleManager.h"
#include "ISettingsModule.h"

#define LOCTEXT_NAMESPACE "EnumViewerProjectSettings"

namespace EnumViewer
{
	namespace EnumViewer
	{
		namespace ProjectSettings
		{
			static const FName ContainerName    = TEXT("Project");
			static const FName CategoryName	    = TEXT("Editor");
			static const FName SectionName      = TEXT("EnumViewer");

			ISettingsModule* GetSettingsModule()
			{
				return FModuleManager::GetModulePtr<ISettingsModule>("Settings");
			}
		}
	}
}

UEnumViewerProjectSettings::UEnumViewerProjectSettings()
{
}

void UEnumViewerProjectSettings::Register()
{
	if (ISettingsModule* SettingsModule = EnumViewer::EnumViewer::ProjectSettings::GetSettingsModule())
	{
		SettingsModule->RegisterSettings(
			EnumViewer::EnumViewer::ProjectSettings::ContainerName,
			EnumViewer::EnumViewer::ProjectSettings::CategoryName,
			EnumViewer::EnumViewer::ProjectSettings::SectionName,
			LOCTEXT("SettingName", "Enum Viewer"),
			LOCTEXT("SettingDescription", "Configure options for the Enum Viewer."),
			GetMutableDefault<UEnumViewerProjectSettings>()
		);
	}
}

void UEnumViewerProjectSettings::Unregister()
{
	if (ISettingsModule* SettingsModule = EnumViewer::EnumViewer::ProjectSettings::GetSettingsModule())
	{
		SettingsModule->UnregisterSettings(
			EnumViewer::EnumViewer::ProjectSettings::ContainerName,
			EnumViewer::EnumViewer::ProjectSettings::CategoryName,
			EnumViewer::EnumViewer::ProjectSettings::SectionName
		);
	}
}

const UEnumViewerProjectSettings& UEnumViewerProjectSettings::Get()
{
	const auto* Settings = GetDefault<UEnumViewerProjectSettings>();
	check(IsValid(Settings));
	return *Settings;
}

#undef LOCTEXT_NAMESPACE
