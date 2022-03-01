// Copyright 2022 Naotsun. All Rights Reserved.

#include "EnumViewer/IEnumViewer.h"
#include "EnumViewer/EnumViewerGlobals.h"
#include "EnumViewer/Utilities/EnumViewerProjectSettings.h"
#include "EnumViewer/Utilities/EnumBrowserTab.h"
#include "EnumViewer/Data/EnumRegistry.h"
#include "EnumViewer/Widgets/SEnumViewer.h"
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
		UEnumViewerProjectSettings::Register();
		
		// Register enum picker tab.
		FEnumBrowserTab::Register();
	}

	void FEnumViewerModule::ShutdownModule()
	{
		// Release the data collected by the enum registry.
		FEnumRegistry::DestroyInstance();
		
		// Unregister enum picker tab.
		FEnumBrowserTab::Unregister();

		// Unregister settings.
		UEnumViewerProjectSettings::Unregister();
	}

	TSharedRef<SWidget> FEnumViewerModule::CreateEnumViewer(
		const FEnumViewerInitializationOptions& InitOptions,
		const FOnEnumPicked& OnEnumPickedDelegate
	)
	{
		return SNew(SEnumViewer, InitOptions)
			.OnEnumPicked(OnEnumPickedDelegate);
	}
}

IMPLEMENT_MODULE(EnumViewer::FEnumViewerModule, EnumViewer)
