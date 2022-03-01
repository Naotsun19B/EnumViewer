// Copyright 2022 Naotsun. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EnumViewer/Data/EnumViewerNode.h"

namespace EnumViewer
{
	class ENUMVIEWER_API FEnumRegistry : public FTickableGameObject
	{
	public:
		// Defines an event to be called when the Enum Registry is updated.
		DECLARE_MULTICAST_DELEGATE(FOnPopulateEnumViewer);

	public:
		// Constructor.
		FEnumRegistry();

		// Destructor.
		virtual ~FEnumRegistry() override;

		// Returns the singleton instance, creating it if required.
		static FEnumRegistry& Get();

		// Returns the singleton instance, or null if it doesn't exist,
		static FEnumRegistry* GetPtr();

		// Destroy the singleton instance.
		static void DestroyInstance();

		// Returns an event that will be called when the Enum Registry is updated.
		FOnPopulateEnumViewer& GetOnPopulateEnumViewer();

		// Returns a list of enums registered in the Enum Registry that meet the conditions.
		TArray<TSharedPtr<FEnumViewerNode>> GetNodeList(
			const TSharedPtr<IPropertyHandle>& InPropertyHandle,
			TFunction<bool(const TSharedPtr<FEnumViewerNode>& EnumViewerNode)> FilterPredicate
		) const;

		// Returns the enum viewer node for the specified enum path.
		TSharedPtr<FEnumViewerNode> FindNodeByEnumPath(const FName& InEnumPath);

	private:
		// FTickableObjectBase interface.
		virtual void Tick(float DeltaTime) override;
		virtual bool IsTickable() const override;
		virtual TStatId GetStatId() const override;
		// End of FTickableObjectBase interface.
		
		// FTickableGameObject interface.
		virtual bool IsTickableWhenPaused() const override;
		virtual bool IsTickableInEditor() const override;
		// End of FTickableGameObject interface.
		
		// Dirty the enum list so it will be rebuilt on the next call to UpdateEnumRegistry.
		void DirtyEnumRegistry();
		
		// Populates the enum list, pulling all the loaded and unloaded enums into a master data list.
		void PopulateEnumRegistry();

		// Called when modules are loaded or unloaded.
		void OnModulesChanged(FName ModuleThatChanged, EModuleChangeReason ReasonForChange);
		
		// Called when hot reload has finished.
		void OnHotReload(bool bWasTriggeredAutomatically);
	
	private:
		// The instance of enum registry singleton that manages the unfiltered enum tree for the Enum Viewer.
		static TUniquePtr<FEnumRegistry> Instance;

		// The event called when the Enum Registry is updated.
		FOnPopulateEnumViewer OnPopulateEnumViewer;

		// Whether the Enum Registry needs to be refreshed.
		bool bRefreshEnumHierarchy = false;

		// The list of enum data collected by the Enum Registry.
		TArray<TSharedPtr<FEnumViewerNode>> EnumNodes;
	};
}
