// Copyright 2022 Naotsun. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class IPropertyHandle;
class UUserDefinedEnum;
struct FAssetData;

namespace EnumViewer
{
	enum class EEnumViewerNameTypeToDisplay : uint8;
	
	/**
	 * Filtered data representing a filtered hierarchy of nodes.
	 */
	class FEnumViewerNode : public TSharedFromThis<FEnumViewerNode>
	{
	public:
		// Create a dummy node.
		FEnumViewerNode();

		// Create a node representing the given enum.
		explicit FEnumViewerNode(const UEnum* InEnum);

		// Create a node representing the given enum asset (may be unloaded).
		explicit FEnumViewerNode(const FAssetData& InEnumAsset);

		// Create a copy of node by specifying the property handle and whether passed the search filter.
		explicit FEnumViewerNode(
			const TSharedPtr<FEnumViewerNode>& Other,
			const TSharedPtr<IPropertyHandle>& InPropertyHandle,
			const bool bInPassedFilter
		);

		// Get the enum that we represent (for loaded enum assets, or native enums).
		const UEnum* GetEnum() const;
		
		// Get the unlocalized name of the enum we represent.
		const FString& GetEnumName() const;

		// Get the localized name of the enum we represent.
		const FText& GetEnumDisplayName() const;

		// Get the display name of the enum we represent, built based on the given option.
		FText GetEnumDisplayName(const EEnumViewerNameTypeToDisplay InNameType) const;

		// Get the full object path to the enum we represent.
		const FName& GetEnumPath() const;

		// Get the enum asset that we represent (for loaded enum assets).
		const UUserDefinedEnum* GetEnumAsset() const;

		// Trigger a load of the enum we represent.
		bool LoadEnum() const;

		// Check whether this enum is restricted for the specific context.
		bool IsRestricted() const;

		// Get the property this filtered node will be working on.
		const TSharedPtr<IPropertyHandle>& GetPropertyHandle() const;

		// Returns whether this enum passed the search filter.
		bool PassedFilter();
		
	private:
		// The enum that we represent (for loaded enum assets, or native enums).
		mutable TWeakObjectPtr<const UEnum> Enum;
		
		// The unlocalized name of the enum we represent.
		FString EnumName;

		// The localized name of the enum we represent.
		mutable FText EnumDisplayName;

		// The full object path to the enum we represent.
		FName EnumPath;

		// The property this filtered node will be working on.
		TSharedPtr<IPropertyHandle> PropertyHandle;

		// Whether this enum passed the search filter.
		bool bPassedFilter;
	};
}