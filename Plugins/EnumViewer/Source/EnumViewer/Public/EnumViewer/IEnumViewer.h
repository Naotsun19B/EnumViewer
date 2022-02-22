// Copyright 2022 Naotsun. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "EnumViewerInitializationOptions.h"

class SWidget;

namespace EnumViewer
{
	// Delegate used with the enum viewer in 'enum picking' mode.
	// You'll bind a delegate when the enum viewer widget is created,
	// which will be fired off when a enum is selected in the list.
	DECLARE_DELEGATE_OneParam(FOnEnumPicked, const UEnum*);

	/**
	 * The public interface to the EnumViewer module.
	 */
	class IEnumViewer : public IModuleInterface
	{
	public:
		// The name of the module for this plugin.
		ENUMVIEWER_API static const FName PluginModuleName;
		
	public:
		// Returns singleton instance, loading the module on demand if needed.
		static IEnumViewer& Get()
		{
			return FModuleManager::LoadModuleChecked<IEnumViewer>(PluginModuleName);
		}

		// Returns whether the module is loaded and ready to use.
		static bool IsAvailable()
		{
			return FModuleManager::Get().IsModuleLoaded(PluginModuleName);
		}

		// Creates a Enum viewer widget.
		virtual TSharedRef<SWidget> CreateEnumViewer(
			const FEnumViewerInitializationOptions& InitOptions,
			const FOnEnumPicked& OnEnumPickedDelegate
		) = 0;
	};
}
