// Copyright 2022 Naotsun. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace EnumViewer
{
	class FEnumBrowserTab
	{
	public:
		// A string that is displayed as the title of the window with the unique name of the tab.
		static const FName TabId;
		
	public:
		// Register-Unregister the enum picker tab in the global tab manager.
		static void Register();
		static void Unregister();

	private:
		// Called when a tab is created.
		static TSharedRef<SDockTab> HandleRegisterTabSpawner(const FSpawnTabArgs& TabSpawnArgs);
	};
}