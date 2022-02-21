// Copyright 2022 Naotsun. All Rights Reserved.

#include "EnumViewer/IEnumViewer.h"
#include "EnumViewer/EnumViewerGlobals.h"
#include "EnumViewer/EnumViewerSettings.h"
#include "EnumViewer/EnumPickerTab.h"
#include "EnumViewer/SEnumViewer.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogEnumViewer);

namespace EnumViewer
{
	const FName IEnumViewer::PluginModuleName = TEXT("EnumViewer");

	class FEnumViewerModule : public IEnumViewer
	{
	public:
		// IModuleInterface interface.
		virtual void StartupModule() override;
		virtual void ShutdownModule() override;
		// End of IModuleInterface interface.

		// IEnumViewer interface.
		virtual TSharedRef<SWidget> CreateEnumViewer(
			const FEnumViewerInitializationOptions& InitOptions,
			const FOnEnumPicked& OnEnumPickedDelegate
		) override;
		// End of IEnumViewer interface.
	};

	void FEnumViewerModule::StartupModule()
	{
		// Register settings.
		UEnumViewerSettings::Register();
		
		// Register enum picker tab.
		FEnumPickerTab::Register();
	}

	void FEnumViewerModule::ShutdownModule()
	{
		// Unregister enum picker tab.
		FEnumPickerTab::Unregister();

		// Unregister settings.
		UEnumViewerSettings::Unregister();
	}

	TSharedRef<SWidget> FEnumViewerModule::CreateEnumViewer(
		const FEnumViewerInitializationOptions& InitOptions,
		const FOnEnumPicked& OnEnumPickedDelegate
	)
	{
		return SNew(SEnumViewer, InitOptions)
			.OnEnumPickedDelegate(OnEnumPickedDelegate);
	}
}

IMPLEMENT_MODULE(EnumViewer::FEnumViewerModule, EnumViewer)
