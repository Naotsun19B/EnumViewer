// Copyright 2022 Naotsun. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Class.h"
#include "EnumViewer/EnumViewerInitializationOptions.h"

struct FAssetData;
class IPropertyHandle;
class UUserDefinedEnum;

namespace EnumViewer
{
	/**
	 * Common data representing an unfiltered hierarchy of nodes.
	 */
	class FEnumViewerNodeData : public TSharedFromThis<FEnumViewerNodeData>
	{
	public:
		// Create a dummy node.
		FEnumViewerNodeData();

		// Create a node representing the given enum.
		explicit FEnumViewerNodeData(const UEnum* InEnum);

		// Create a node representing the given enum asset (may be unloaded).
		explicit FEnumViewerNodeData(const FAssetData& InEnumAsset);

		// Get the unlocalized name of the enum we represent.
		const FString& GetEnumName() const;

		// Get the localized name of the enum we represent.
		FText GetEnumDisplayName() const;

		// Get the full object path to the enum we represent.
		FName GetEnumPath() const;

		// Get the enum that we represent (for loaded enum assets, or native enums).
		const UEnum* GetEnum() const;

		// Get the enum asset that we represent (for loaded enum assets).
		const UUserDefinedEnum* GetEnumAsset() const;

		// Trigger a load of the enum we represent.
		bool LoadEnum() const;

	private:
		// The unlocalized name of the enum we represent.
		FString EnumName;

		// The localized name of the enum we represent.
		mutable FText EnumDisplayName;

		// The full object path to the enum we represent.
		FName EnumPath;

		// The enum that we represent (for loaded enum assets, or native enums).
		mutable TWeakObjectPtr<const UEnum> Enum;
	};

	/**
	 * Filtered data representing a filtered hierarchy of nodes.
	 */
	class FEnumViewerNode : public TSharedFromThis<FEnumViewerNode>
	{
	public:
		// Predicate function that can be used to sort instances by enum name.
		static bool SortPredicate(const TSharedPtr<FEnumViewerNode>& InA, const TSharedPtr<FEnumViewerNode>& InB);
	
		// Create a dummy node.
		FEnumViewerNode();

		// Create a node representing the given data.
		FEnumViewerNode(const TSharedRef<FEnumViewerNodeData>& InData, const TSharedPtr<IPropertyHandle>& InPropertyHandle, const bool bInPassedFilter);

		// Get the unlocalized name of the enum we represent.
		const FString& GetEnumName() const;

		// Get the localized name of the enum we represent.
		FText GetEnumDisplayName() const;

		// Get the display name of the enum we represent, built based on the given option.
		FText GetEnumDisplayName(const EnumViewer::EEnumViewerNameTypeToDisplay InNameType) const;

		// Get the full object path to the enum we represent/
		FName GetEnumPath() const;

		// Get the enum that we represent (for loaded enum assets, or native enums).
		const UEnum* GetEnum() const;

		// Get the enum asset that we represent (for loaded enum assets).
		const UUserDefinedEnum* GetEnumAsset() const;

		// Trigger a load of the enum we represent.
		bool LoadEnum() const;

		// Check whether this enum is restricted for the specific context.
		bool IsRestricted() const;

		// Get the property this filtered node will be working on.
		const TSharedPtr<IPropertyHandle>& GetPropertyHandle() const;
	
		// True if the enum passed the filter.
		bool PassedFilter() const;

		// True if the enum passed the filter.
		void PassedFilter(const bool bInPassedFilter);

	private:
		// Reference to the common data this filtered node represents.
		TSharedRef<const FEnumViewerNodeData> NodeData;

		// The property this filtered node will be working on.
		TSharedPtr<IPropertyHandle> PropertyHandle;

		// True if the enum passed the filter.
		bool bPassedFilter;
	};
}
