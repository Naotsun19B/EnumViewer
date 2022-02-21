// Copyright 2022 Naotsun. All Rights Reserved.

#include "EnumPickerTab.h"
#include "EnumViewer/IEnumViewer.h"
#include "EnumViewer/EnumViewerInitializationOptions.h"
#include "EnumViewer/SEnumViewer.h"
#include "Widgets/Docking/SDockTab.h"
#include "WorkspaceMenuStructure/Public/WorkspaceMenuStructureModule.h"
#include "WorkspaceMenuStructure/Public/WorkspaceMenuStructure.h"

#define LOCTEXT_NAMESPACE "EnumPickerTab"

namespace EnumViewer
{
	const FName FEnumPickerTab::TabId = TEXT("EnumPicker");
	
	void FEnumPickerTab::Register()
	{
		const TSharedRef<FGlobalTabmanager>& GlobalTabManager = FGlobalTabmanager::Get();
		GlobalTabManager->RegisterNomadTabSpawner(
			TabId,
			FOnSpawnTab::CreateStatic(&FEnumPickerTab::HandleRegisterTabSpawner)
		)
		.SetDisplayName(LOCTEXT("TabTitle", "Enum Viewer"))
		.SetTooltipText(LOCTEXT("TooltipText", "Displays all enums that exist within this project."))
		.SetGroup(WorkspaceMenu::GetMenuStructure().GetDeveloperToolsMiscCategory())
		.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "ClassViewer.TabIcon"));
	}

	void FEnumPickerTab::Unregister()
	{
		if (FSlateApplication::IsInitialized())
		{
			const TSharedRef<FGlobalTabmanager> GlobalTabManager = FGlobalTabmanager::Get();
			GlobalTabManager->UnregisterTabSpawner(TabId);
		}
	}

	TSharedRef<SDockTab> FEnumPickerTab::HandleRegisterTabSpawner(const FSpawnTabArgs& TabSpawnArgs)
	{
		FEnumViewerInitializationOptions InitOptions;
		InitOptions.Mode = EEnumViewerMode::EnumBrowsing;
		
		return SNew(SDockTab)
			.TabRole(ETabRole::NomadTab)
			[
				SNew(SEnumViewer, InitOptions)
				.OnEnumPickedDelegate(FOnEnumPicked())
			];
	}
}

#undef LOCTEXT_NAMESPACE
