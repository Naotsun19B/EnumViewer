// Copyright 2022 Naotsun. All Rights Reserved.

#include "EnumViewerNode.h"
#include "AssetData.h"
#include "PropertyHandle.h"
#include "Misc/ScopedSlowTask.h"
#include "Engine/UserDefinedEnum.h"

#define LOCTEXT_NAMESPACE "EnumViewer"

namespace EnumViewer
{
	FEnumViewerNodeData::FEnumViewerNodeData()
		: EnumName(TEXT("None"))
		, EnumDisplayName(LOCTEXT("None", "None"))
	{
	}

	FEnumViewerNodeData::FEnumViewerNodeData(const UEnum* InEnum)
		: EnumName(InEnum->GetName())
		, EnumDisplayName(InEnum->GetName())
		, EnumPath(*InEnum->GetPathName())
		, Enum(InEnum)
	{
	}

	FEnumViewerNodeData::FEnumViewerNodeData(const FAssetData& InEnumAsset)
		: EnumName(InEnumAsset.AssetName.ToString())
		, EnumPath(InEnumAsset.ObjectPath)
	{
		// Attempt to find the enum asset in the case where it's already been loaded
		Enum = FindObject<UEnum>(ANY_PACKAGE, *EnumPath.ToString());

		// Cache the resolved display name if available, or synthesize one if the enum asset is unloaded
		EnumDisplayName = Enum.IsValid()
			? Enum->GetName()
			: FText::AsCultureInvariant(FName::NameToDisplayString(EnumName, false));
	}

	const FString& FEnumViewerNodeData::GetEnumName() const
	{
		return EnumName;
	}

	FText FEnumViewerNodeData::GetEnumDisplayName() const
	{
		return EnumDisplayName;
	}

	FName FEnumViewerNodeData::GetEnumPath() const
	{
		return EnumPath;
	}

	const UEnum* FEnumViewerNodeData::GetEnum() const
	{
		return Enum.Get();
	}

	const UUserDefinedEnum* FEnumViewerNodeData::GetEnumAsset() const
	{
		return Cast<UUserDefinedEnum>(Enum.Get());
	}

	bool FEnumViewerNodeData::LoadEnum() const
	{
		if (Enum.IsValid())
		{
			return true;
		}

		// Attempt to load the enum
		if (!EnumPath.IsNone())
		{
			FScopedSlowTask SlowTask(0.0f, LOCTEXT("LoadingEnum", "Loading Enum..."));
			SlowTask.MakeDialogDelayed(1.0f);

			Enum = LoadObject<UEnum>(nullptr, *EnumPath.ToString());
		}

		// Re-cache the resolved display name as it may be different than the one we synthesized for an unloaded enum asset
		if (Enum.IsValid())
		{
			EnumDisplayName = FText::FromString(Enum->GetName());
			return true;
		}

		return false;
	}

	bool FEnumViewerNode::SortPredicate(const TSharedPtr<FEnumViewerNode>& InA, const TSharedPtr<FEnumViewerNode>& InB)
	{
		check(InA.IsValid());
		check(InB.IsValid());

		const FString& AString = InA->GetEnumName();
		const FString& BString = InB->GetEnumName();

		return AString < BString;
	}

	FEnumViewerNode::FEnumViewerNode()
		: NodeData(MakeShared<FEnumViewerNodeData>())
		, bPassedFilter(true)
	{
	}

	FEnumViewerNode::FEnumViewerNode(const TSharedRef<FEnumViewerNodeData>& InData, const TSharedPtr<IPropertyHandle>& InPropertyHandle, const bool InPassedFilter)
		: NodeData(InData)
		, PropertyHandle(InPropertyHandle)
		, bPassedFilter(InPassedFilter)
	{
	}

	const FString& FEnumViewerNode::GetEnumName() const
	{
		return NodeData->GetEnumName();
	}

	FText FEnumViewerNode::GetEnumDisplayName() const
	{
		return NodeData->GetEnumDisplayName();
	}

	FText FEnumViewerNode::GetEnumDisplayName(const EEnumViewerNameTypeToDisplay InNameType) const
	{
		switch (InNameType)
		{
		case EEnumViewerNameTypeToDisplay::EnumName:
		{
			return FText::AsCultureInvariant(GetEnumName());
		}
		case EEnumViewerNameTypeToDisplay::DisplayName:
		{
			return GetEnumDisplayName();
		}
		case EEnumViewerNameTypeToDisplay::Dynamic:
		{
			const FText BasicName = FText::AsCultureInvariant(GetEnumName());
			const FText DisplayName = GetEnumDisplayName();
			const FString SynthesizedDisplayName = FName::NameToDisplayString(BasicName.ToString(), false);

			// Only show both names if we have a display name set that is different from the basic name and not synthesized from it
			return (DisplayName.IsEmpty() || DisplayName.ToString().Equals(BasicName.ToString()) || DisplayName.ToString().Equals(SynthesizedDisplayName))
				? BasicName
				: FText::Format(LOCTEXT("EnumDynamicDisplayNameFmt", "{0} ({1})"), BasicName, DisplayName);
		}
		default:
			break;
		}

		ensureMsgf(false, TEXT("FEnumViewerNode::GetEnumName called with invalid name type."));
		return GetEnumDisplayName();
	}

	FName FEnumViewerNode::GetEnumPath() const
	{
		return NodeData->GetEnumPath();
	}

	const UEnum* FEnumViewerNode::GetEnum() const
	{
		return NodeData->GetEnum();
	}

	const UUserDefinedEnum* FEnumViewerNode::GetEnumAsset() const
	{
		return NodeData->GetEnumAsset();
	}

	bool FEnumViewerNode::LoadEnum() const
	{
		return NodeData->LoadEnum();
	}

	bool FEnumViewerNode::IsRestricted() const
	{
		return PropertyHandle && PropertyHandle->IsRestricted(GetEnumName());
	}

	const TSharedPtr<IPropertyHandle>& FEnumViewerNode::GetPropertyHandle() const
	{
		return PropertyHandle;
	}

	bool FEnumViewerNode::PassedFilter() const
	{
		return bPassedFilter;
	}

	void FEnumViewerNode::PassedFilter(const bool bInPassedFilter)
	{
		bPassedFilter = bInPassedFilter;
	}
}

#undef LOCTEXT_NAMESPACE
