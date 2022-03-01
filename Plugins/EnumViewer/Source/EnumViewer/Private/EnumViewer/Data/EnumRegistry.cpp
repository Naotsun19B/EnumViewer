// Copyright 2022 Naotsun. All Rights Reserved.

#include "EnumViewer/Data/EnumRegistry.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/HotReloadInterface.h"
#include "Misc/ScopedSlowTask.h"
#include "Engine/UserDefinedEnum.h"

#define LOCTEXT_NAMESPACE "EnumRegistry"

namespace EnumViewer
{
	DECLARE_STATS_GROUP(TEXT("EnumRegistry"), STATGROUP_EnumRegistry, STATCAT_Advanced);
	
	FEnumRegistry::FEnumRegistry()
	{
		// Bind to the event when the Enum Registry is updated.
		FAssetRegistryModule::GetRegistry().OnFilesLoaded().AddRaw(this, &FEnumRegistry::DirtyEnumRegistry);
		FModuleManager::Get().OnModulesChanged().AddRaw(this, &FEnumRegistry::OnModulesChanged);
		if (auto* HotReload = IHotReloadInterface::GetPtr())
		{
			HotReload->OnHotReload().AddRaw(this, &FEnumRegistry::OnHotReload);
		}

		PopulateEnumRegistry();
	}

	FEnumRegistry::~FEnumRegistry()
	{
		if (FModuleManager::Get().IsModuleLoaded(TEXT("HotReload")))
		{
			if (auto* HotReload = IHotReloadInterface::GetPtr())
			{
				HotReload->OnHotReload().RemoveAll(this);
			}
		}
		
		if (FModuleManager::Get().IsModuleLoaded(TEXT("AssetRegistry")))
		{
			FAssetRegistryModule::GetRegistry().OnFilesLoaded().RemoveAll(this);
		}

		FModuleManager::Get().OnModulesChanged().RemoveAll(this);
	}
	
	FEnumRegistry& FEnumRegistry::Get()
	{
		if (!Instance.IsValid())
		{
			Instance = MakeUnique<FEnumRegistry>();
			Instance->PopulateEnumRegistry();
		}
		
		return *Instance;
	}

	FEnumRegistry* FEnumRegistry::GetPtr()
	{
		return Instance.Get();
	}

	void FEnumRegistry::DestroyInstance()
	{
		Instance.Reset();
	}

	FEnumRegistry::FOnPopulateEnumViewer& FEnumRegistry::GetOnPopulateEnumViewer()
	{
		return OnPopulateEnumViewer;
	}

	TArray<TSharedPtr<FEnumViewerNode>> FEnumRegistry::GetNodeList(
		const TSharedPtr<IPropertyHandle>& InPropertyHandle,
		TFunction<bool(const TSharedPtr<FEnumViewerNode>& EnumViewerNode)> FilterPredicate
	) const
	{
		TArray<TSharedPtr<FEnumViewerNode>> FilteredEnumNodes;
		for (const auto& EnumNode : EnumNodes)
		{
			if (!EnumNode.IsValid())
			{
				continue;
			}
			
			FilteredEnumNodes.Add(
				MakeShared<FEnumViewerNode>(
					EnumNode,
					InPropertyHandle,
					FilterPredicate(EnumNode)
				)
			);
		}
			
		return FilteredEnumNodes;
	}

	TSharedPtr<FEnumViewerNode> FEnumRegistry::FindNodeByEnumPath(const FName& InEnumPath)
	{
		const TSharedPtr<FEnumViewerNode>* FoundEnumPtr = EnumNodes.FindByPredicate(
				[&InEnumPath](const TSharedPtr<FEnumViewerNode>& EnumNodePtr)
				{
					if (EnumNodePtr.IsValid())
					{
						return (EnumNodePtr->GetEnumPath() == InEnumPath);
					}

					return false;
				}
			);

		if (FoundEnumPtr != nullptr)
		{
			return *FoundEnumPtr;
		}

		return nullptr;
	}

	void FEnumRegistry::Tick(float DeltaTime)
	{
		bRefreshEnumHierarchy = false;
		PopulateEnumRegistry();
	}

	bool FEnumRegistry::IsTickable() const
	{
		return bRefreshEnumHierarchy;
	}

	TStatId FEnumRegistry::GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FEnumRegistry, STATGROUP_EnumRegistry);
	}

	bool FEnumRegistry::IsTickableWhenPaused() const
	{
		return true;
	}

	bool FEnumRegistry::IsTickableInEditor() const
	{
		return true;
	}

	void FEnumRegistry::DirtyEnumRegistry()
	{
		bRefreshEnumHierarchy = true;
	}

	void FEnumRegistry::PopulateEnumRegistry()
	{
		FScopedSlowTask SlowTask(0.0f, LOCTEXT("RebuildingEnumRegistry", "Rebuilding Enum Registry"));
		SlowTask.MakeDialog();

		auto AddUnique = [this](const TSharedPtr<FEnumViewerNode>& NewEnumViewerNodeData)
		{
			const auto* FoundEnumNode = EnumNodes.FindByPredicate(
				[&NewEnumViewerNodeData](const TSharedPtr<FEnumViewerNode>& EnumViewerNodeData) -> bool
				{
					if (EnumViewerNodeData.IsValid() && NewEnumViewerNodeData.IsValid())
					{
						return (EnumViewerNodeData->GetEnumPath() == NewEnumViewerNodeData->GetEnumPath());
					}

					return false;
				}
			);

			if (FoundEnumNode == nullptr)
			{
				EnumNodes.Add(NewEnumViewerNodeData);
			}
		};
		
		// Go through all of the enums and see if they should be added to the list.
		for (const auto* Enum : TObjectRange<UEnum>())
		{
			if (IsValid(Enum))
			{
				const TSharedPtr<FEnumViewerNode> EnumViewerNodeData = MakeShared<FEnumViewerNode>(Enum);
				AddUnique(EnumViewerNodeData);
			}
		}

		// Add any enum assets directly under the root (since they don't support inheritance)
		{
			const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

			FARFilter Filter;
			Filter.ClassNames.Add(UUserDefinedEnum::StaticClass()->GetFName());
			Filter.bRecursiveClasses = true;

			TArray<FAssetData> UserDefinedEnumsList;
			AssetRegistryModule.Get().GetAssets(Filter, UserDefinedEnumsList);
			
			for (const FAssetData& UserDefinedEnumData : UserDefinedEnumsList)
			{
				const TSharedPtr<FEnumViewerNode> EnumViewerNodeData = MakeShared<FEnumViewerNode>(UserDefinedEnumData);
				AddUnique(EnumViewerNodeData);
			}
		}
		
		// All viewers must refresh.
		OnPopulateEnumViewer.Broadcast();
	}

	void FEnumRegistry::OnModulesChanged(FName ModuleThatChanged, EModuleChangeReason ReasonForChange)
	{
		if (ReasonForChange == EModuleChangeReason::ModuleLoaded ||
			ReasonForChange == EModuleChangeReason::ModuleUnloaded)
		{
			DirtyEnumRegistry();
		}
	}

	void FEnumRegistry::OnHotReload(bool bWasTriggeredAutomatically)
	{
		DirtyEnumRegistry();
	}

	TUniquePtr<FEnumRegistry> FEnumRegistry::Instance;
}

#undef LOCTEXT_NAMESPACE
