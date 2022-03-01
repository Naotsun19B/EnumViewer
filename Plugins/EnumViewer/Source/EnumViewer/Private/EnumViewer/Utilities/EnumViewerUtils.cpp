// Copyright 2022 Naotsun. All Rights Reserved.

#include "EnumViewer/Utilities/EnumViewerUtils.h"
#include "EnumViewer/Types/EnumViewerInitializationOptions.h"
#include "EnumViewer/Types/EnumViewerFilter.h"
#include "SourceCodeNavigation.h"
#include "Engine/UserDefinedEnum.h"
#include "Misc/FileHelper.h"

#define LOCTEXT_NAMESPACE "EnumViewerUtils"

namespace EnumViewer
{
	FText FEnumViewerUtils::GetEnumDisplayName(const TWeakObjectPtr<const UEnum> InEnum)
	{
		if (ensure(InEnum.IsValid()))
		{
			return FText::FromString(InEnum->GetMetaData(TEXT("DisplayName")));
		}

		return FText::GetEmpty();
	}

	bool FEnumViewerUtils::IsEnumAllowed(
		const FEnumViewerInitializationOptions& InInitOptions,
		const TWeakObjectPtr<const UEnum> InEnum
	)
	{
		const TSharedPtr<IEnumViewerFilter> EnumFilter = InInitOptions.EnumFilter;
		if (EnumFilter.IsValid())
		{
			return EnumFilter->IsEnumAllowed(InInitOptions, InEnum.Get());
		}
		
		return true;
	}

	bool FEnumViewerUtils::IsUnloadedEnumAllowed(
		const FEnumViewerInitializationOptions& InInitOptions,
		const FName InEnumPath
	)
	{
		const TSharedPtr<IEnumViewerFilter> EnumFilter = InInitOptions.EnumFilter;
		if (EnumFilter.IsValid())
		{
			return EnumFilter->IsUnloadedEnumAllowed(InInitOptions, InEnumPath);
		}
		
		return true;
	}

	void FEnumViewerUtils::OpenEnumInIDE(const UEnum* InEnum)
	{
		if (!IsValid(InEnum))
		{
			return;
		}

		FString EnumHeaderPath;
		if (FSourceCodeNavigation::FindClassHeaderPath(InEnum, EnumHeaderPath) && IFileManager::Get().FileSize(*EnumHeaderPath) != INDEX_NONE)
		{
			const FString AbsoluteHeaderPath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*EnumHeaderPath);
			FSourceCodeNavigation::OpenSourceFile(AbsoluteHeaderPath);
		}
	}

	void FEnumViewerUtils::OpenAssetEditor(const UUserDefinedEnum* InEnum)
	{
		if (!IsValid(GEditor) || !IsValid(InEnum))
		{
			return;
		}

		if (auto* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
		{
			AssetEditorSubsystem->OpenEditorForAsset(const_cast<UUserDefinedEnum*>(InEnum));
		}
	}

	void FEnumViewerUtils::FindInContentBrowser(const UEnum* InEnum)
	{
		if (!IsValid(GEditor) || !IsValid(InEnum))
		{
			return;
		}

		TArray<UObject*> Objects = { const_cast<UEnum*>(InEnum) };
		GEditor->SyncBrowserToObjects(Objects);
	}

	TSharedRef<SWidget> FEnumViewerUtils::GenerateContextMenuWidget(const UEnum* InEnum)
	{
		// Empty list of commands.
		const TSharedPtr<FUICommandList> Commands;

		// Set the menu to automatically close when the user commits to a choice
		FMenuBuilder MenuBuilder(true, Commands);
		if (const UUserDefinedEnum* UserDefinedEnum = Cast<UUserDefinedEnum>(InEnum))
		{
			MenuBuilder.AddMenuEntry(
				LOCTEXT("EditEnumAssetTitle", "Edit Enum..."), 
				LOCTEXT("EditEnumAssetTooltip", "Open the enum in the asset editor."), 
				FSlateIcon(), 
				FUIAction(FExecuteAction::CreateStatic(&FEnumViewerUtils::OpenAssetEditor, UserDefinedEnum))
			);

			MenuBuilder.AddMenuEntry(
				LOCTEXT("FindContentTitle", "Find in Content Browser..."), 
				LOCTEXT("FindContentTooltip", "Find in Content Browser"), 
				FSlateIcon(), 
				FUIAction(FExecuteAction::CreateStatic(&FEnumViewerUtils::FindInContentBrowser, InEnum))
			);
		}
		else
		{
			MenuBuilder.AddMenuEntry(
				LOCTEXT("OpenSourceCodeTitle", "Open Source Code..."), 
				LOCTEXT("OpenSourceCodeTooltip", "Open the source file for this enum in the IDE."), 
				FSlateIcon(), 
				FUIAction(FExecuteAction::CreateStatic(&FEnumViewerUtils::OpenEnumInIDE, InEnum))
			);
		}
				
		return MenuBuilder.MakeWidget();
	}
}

#undef LOCTEXT_NAMESPACE
