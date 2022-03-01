// Copyright 2022 Naotsun. All Rights Reserved.

#include "EnumViewer/Data/EnumViewerNode.h"
#include "EnumViewer/Utilities/EnumViewerUtils.h"
#include "EnumViewer/Types/EnumViewerInitializationOptions.h"
#include "Engine/UserDefinedEnum.h"
#include "Misc/ScopedSlowTask.h"

#define LOCTEXT_NAMESPACE "EnumViewerNode"

namespace EnumViewer
{
	FEnumViewerNode::FEnumViewerNode()
		: EnumName(TEXT("None"))
		, EnumDisplayName(LOCTEXT("EmptyDisplayName", "None"))
	{
	}

	FEnumViewerNode::FEnumViewerNode(const UEnum* InEnum)
		: Enum(InEnum)
	{
		check(Enum.IsValid());

		EnumName = Enum->GetName();
		EnumDisplayName = FEnumViewerUtils::GetEnumDisplayName(Enum);
		EnumPath = *Enum->GetPathName();
	}

	FEnumViewerNode::FEnumViewerNode(const FAssetData& InEnumAsset)
		: EnumName(InEnumAsset.AssetName.ToString())
		, EnumPath(InEnumAsset.ObjectPath)
	{
		// Attempt to find the enum asset in the case where it's already been loaded.
		Enum = FindObject<UEnum>(ANY_PACKAGE, *EnumPath.ToString());

		// Cache the resolved display name if available, or synthesize one if the enum asset is unloaded.
		if (Enum.IsValid())
		{
			EnumDisplayName = FText::FromString(Enum->GetMetaData(TEXT("DisplayName")));
		}
		else
		{
			EnumDisplayName = FText::AsCultureInvariant(FName::NameToDisplayString(EnumName, false));
		}
	}

	FEnumViewerNode::FEnumViewerNode(
		const TSharedPtr<FEnumViewerNode>& Other,
		const TSharedPtr<IPropertyHandle>& InPropertyHandle,
		const bool bInPassedFilter
	)
		: PropertyHandle(InPropertyHandle)
		, bPassedFilter(bInPassedFilter)
	{
		if (Other.IsValid())
		{
			Enum = Other->Enum;
			EnumName = Other->EnumName;
			EnumDisplayName = Other->EnumDisplayName;
			EnumPath = Other->EnumPath;
		}
	}

	const UEnum* FEnumViewerNode::GetEnum() const
	{
		return Enum.Get();
	}

	const FString& FEnumViewerNode::GetEnumName() const
	{
		return EnumName;
	}

	const FText& FEnumViewerNode::GetEnumDisplayName() const
	{
		return EnumDisplayName;
	}

	FText FEnumViewerNode::GetEnumDisplayName(const EEnumViewerNameTypeToDisplay InNameType) const
	{
		FText ReturnValue = FText::GetEmpty();
		switch (InNameType)
		{
		case EEnumViewerNameTypeToDisplay::EnumName:
		{
			ReturnValue = FText::AsCultureInvariant(GetEnumName());
			break;
		}
		case EEnumViewerNameTypeToDisplay::DisplayName:
		{
			ReturnValue = GetEnumDisplayName();
			break;
		}
		case EEnumViewerNameTypeToDisplay::Dynamic:
		{
			const FString DisplayName = EnumDisplayName.ToString();
			const FString SynthesizedDisplayName = FName::NameToDisplayString(EnumName, false);
			if (DisplayName.IsEmpty() ||
				DisplayName.Equals(EnumName) ||
				DisplayName.Equals(SynthesizedDisplayName))
			{
				ReturnValue = FText::FromString(EnumName);
			}
			else
			{
				ReturnValue = FText::Format(
					LOCTEXT("EnumDynamicDisplayNameFormat", "{0} ({1})"),
					FText::FromString(EnumName),
					FText::FromString(DisplayName)
				);
			}
			break;
		}
		default:
			ensureMsgf(false, TEXT("FEnumViewerNode::GetEnumName called with invalid name type."));
			break;
		}

		return ReturnValue;
	}

	const FName& FEnumViewerNode::GetEnumPath() const
	{
		return EnumPath;
	}

	const UUserDefinedEnum* FEnumViewerNode::GetEnumAsset() const
	{
		return Cast<UUserDefinedEnum>(Enum.Get());
	}

	bool FEnumViewerNode::LoadEnum() const
	{
		if (Enum.IsValid())
		{
			return true;
		}

		// Attempt to load the enum.
		if (!EnumPath.IsNone())
		{
			FScopedSlowTask SlowTask(0.0f, LOCTEXT("LoadingEnum", "Loading Enum..."));
			SlowTask.MakeDialogDelayed(1.0f);

			Enum = LoadObject<UEnum>(nullptr, *EnumPath.ToString());
		}

		// Re-cache the resolved display name as it may be different than the one
		// we synthesized for an unloaded enum asset.
		if (Enum.IsValid())
		{
			EnumDisplayName = FEnumViewerUtils::GetEnumDisplayName(Enum);
			return true;
		}

		return false;
	}

	bool FEnumViewerNode::IsRestricted() const
	{
		if (PropertyHandle.IsValid())
		{
			return PropertyHandle->IsRestricted(GetEnumName());
		}

		return false;
	}

	const TSharedPtr<IPropertyHandle>& FEnumViewerNode::GetPropertyHandle() const
	{
		return PropertyHandle;
	}

	bool FEnumViewerNode::PassedFilter()
	{
		return bPassedFilter;
	}
}

#undef LOCTEXT_NAMESPACE
