// Copyright Epic Games, Inc. All Rights Reserved.

#include "EnumViewerPlugin.h"
#include "Modules/ModuleManager.h"
#include "EnumViewer/IEnumViewer.h"
#include "EnumViewer/EnumViewerGlobals.h"
#include "EnumViewer/Types/EnumViewerFilter.h"

namespace EnumViewer
{
	static FAutoConsoleCommand EnumPickerTest
	(
		TEXT("EnumPickerTest"),
		TEXT("Start enum picker and output the selected enum to the log."),
		FConsoleCommandDelegate::CreateLambda(
			[]()
			{
				class FEnumPickerFilter : public IEnumViewerFilter
				{
				protected:
					// IEnumViewerFilter interface.
					virtual bool IsEnumAllowed(const FEnumViewerInitializationOptions& InInitOptions, const UEnum* InEnum) override
					{
						return true;
					}
					virtual bool IsUnloadedEnumAllowed(const FEnumViewerInitializationOptions& InInitOptions, const FName InEnumPath) override
					{
						return true;
					}
					// End of IEnumViewerFilter interface.
				};

				const TSharedPtr<SWindow> Window = SNew(SWindow)
					.Title(NSLOCTEXT("EnumPickerTest", "WindowTitle", "Enum Picker"))
					.SizingRule(ESizingRule::Autosized);
				
				FEnumViewerInitializationOptions Options;
				Options.EnumFilter = MakeShared<FEnumPickerFilter>();
				Options.NameTypeToDisplay = EEnumViewerNameTypeToDisplay::Dynamic;
				
				Window->SetContent(
					IEnumViewer::Get().CreateEnumViewer(
						Options,
						FOnEnumPicked::CreateLambda(
							[Window](const UEnum* PickedEnum)
							{
								if (Window.IsValid())
								{
									Window->RequestDestroyWindow();
								}
								
								UE_LOG(LogEnumViewer, Log, TEXT("Picked Enum : %s"), *GetFullNameSafe(PickedEnum));
							}
						)
					)
				);
				
				FSlateApplication::Get().AddModalWindow(Window.ToSharedRef(), nullptr, false);
			}
		)
	);
}

IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl, EnumViewerPlugin, "EnumViewerPlugin");
