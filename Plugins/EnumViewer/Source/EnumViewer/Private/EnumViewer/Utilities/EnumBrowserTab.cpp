// Copyright 2022 Naotsun. All Rights Reserved.

#include "EnumBrowserTab.h"
#include "EnumViewer/Types/EnumViewerInitializationOptions.h"
#include "EnumViewer/Widgets/SEnumViewer.h"
#include "Widgets/Docking/SDockTab.h"
#include "WorkspaceMenuStructureModule.h"
#include "WorkspaceMenuStructure.h"

#define LOCTEXT_NAMESPACE "EnumPickerTab"

namespace EnumViewer
{
	const FName FEnumBrowserTab::TabId = TEXT("EnumPicker");
	
	void FEnumBrowserTab::Register()
	{
		const TSharedRef<FGlobalTabmanager>& GlobalTabManager = FGlobalTabmanager::Get();
		GlobalTabManager->RegisterNomadTabSpawner(
			TabId,
			FOnSpawnTab::CreateStatic(&FEnumBrowserTab::HandleRegisterTabSpawner)
		)
		.SetDisplayName(LOCTEXT("TabTitle", "Enum Viewer"))
		.SetTooltipText(LOCTEXT("TooltipText", "Displays all enums that exist within this project."))
		.SetGroup(WorkspaceMenu::GetMenuStructure().GetDeveloperToolsMiscCategory())
		.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "ClassViewer.TabIcon"));
	}

	void FEnumBrowserTab::Unregister()
	{
		if (FSlateApplication::IsInitialized())
		{
			const TSharedRef<FGlobalTabmanager> GlobalTabManager = FGlobalTabmanager::Get();
			GlobalTabManager->UnregisterTabSpawner(TabId);
		}
	}

	TSharedRef<SDockTab> FEnumBrowserTab::HandleRegisterTabSpawner(const FSpawnTabArgs& TabSpawnArgs)
	{
		FEnumViewerInitializationOptions InitOptions;
		InitOptions.Mode = EEnumViewerMode::EnumBrowsing;
		
		return SNew(SDockTab)
			.TabRole(NomadTab)
			[
				SNew(SEnumViewer, InitOptions)
			];
	}
}

#undef LOCTEXT_NAMESPACE
