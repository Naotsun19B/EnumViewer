// Copyright 2022 Naotsun. All Rights Reserved.

#include "EnumViewer/Widgets/SEnumViewerRow.h"
#include "EnumViewer/Utilities/EnumViewerUtils.h"
#include "EnumViewer/Data/EnumViewerNode.h"
#include "Engine/UserDefinedEnum.h"
#include "IDocumentation.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SComboButton.h"

namespace EnumViewer
{
	void SEnumViewerRow::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
	{
		EnumDisplayName = InArgs._EnumDisplayName;
		bIsInEnumBrowser = InArgs._bIsInEnumViewer;
		bDynamicEnumLoading = InArgs._bDynamicEnumLoading;
		TextColor = InArgs._TextColor;
		AssociatedNode = InArgs._AssociatedNode;
		OnDoubleClicked = InArgs._OnDoubleClicked;
		
		bool bIsRestricted = false;
		if (AssociatedNode.IsValid())
		{
			bIsRestricted = AssociatedNode->IsRestricted();
		}
		
		ChildSlot
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding(0.0f, 3.0f, 6.0f, 3.0f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(EnumDisplayName)
				.HighlightText(InArgs._HighlightText)
				.ColorAndOpacity(this, &SEnumViewerRow::GetTextColor)
				.ToolTip(GetTextTooltip())
				.IsEnabled(!bIsRestricted)
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, 6.0f, 0.0f)
			[
				SNew(SComboButton)
				.ContentPadding(FMargin(2.0f))
				.Visibility(this, &SEnumViewerRow::GetOptionsVisibility)
				.OnGetMenuContent(this, &SEnumViewerRow::GenerateOptionsMenu)
			]
		];

		ConstructInternal(
			STableRow::FArguments()
			.ShowSelection(true)
			.OnDragDetected(InArgs._OnDragDetected),
			InOwnerTableView
		);
	}

	FReply SEnumViewerRow::OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent)
	{
		if (bIsInEnumBrowser)
		{
			if (AssociatedNode.IsValid())
			{
				// If in a Enum Viewer and it has not been loaded, load the enum when double-left clicking.
				if (bDynamicEnumLoading && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
				{
					AssociatedNode->LoadEnum();
				}

				// If there is a enum asset, open its asset editor; otherwise try to open the enum header.
				if (const UUserDefinedEnum* UserDefinedEnum = AssociatedNode->GetEnumAsset())
				{
					FEnumViewerUtils::OpenAssetEditor(UserDefinedEnum);
				}
				else if (const UEnum* Enum = AssociatedNode->GetEnum())
				{
					FEnumViewerUtils::OpenEnumInIDE(Enum);
				}
			}
		}
		else
		{
			OnDoubleClicked.ExecuteIfBound(AssociatedNode);
		}

		return FReply::Handled();
	}

	FSlateColor SEnumViewerRow::GetTextColor() const
	{
		const TSharedPtr<ITypedTableView<TSharedPtr<FString>>> OwnerWidget = OwnerTablePtr.Pin();
		const TSharedPtr<FString>* Item = OwnerWidget->Private_ItemFromWidget(this);
		if (OwnerWidget->Private_IsItemSelected(*Item))
		{
			return FSlateColor::UseForeground();
		}

		return TextColor;
	}

	TSharedPtr<IToolTip> SEnumViewerRow::GetTextTooltip() const
	{
		TSharedPtr<IToolTip> TextToolTip;

		const TSharedPtr<IPropertyHandle> PropertyHandle = AssociatedNode->GetPropertyHandle();
		if (PropertyHandle && AssociatedNode->IsRestricted())
		{
			FText RestrictionToolTip;
			PropertyHandle->GenerateRestrictionToolTip(
				*AssociatedNode->GetEnumName(),
				RestrictionToolTip
			);

			TextToolTip = IDocumentation::Get()->CreateToolTip(
				RestrictionToolTip,
				nullptr,
				{},
				{}
			);
		}
		else if (!AssociatedNode->GetEnumPath().IsNone())
		{
			TextToolTip = SNew(SToolTip)
				.Text(FText::FromName(AssociatedNode->GetEnumPath()));
		}

		return TextToolTip;
	}

	EVisibility SEnumViewerRow::GetOptionsVisibility() const
	{
		if (bIsInEnumBrowser)
		{
			if (AssociatedNode.IsValid() && IsValid(AssociatedNode->GetEnum()))
			{
				return EVisibility::Visible;
			}
		}

		return EVisibility::Collapsed;
	}

	TSharedRef<SWidget> SEnumViewerRow::GenerateOptionsMenu() const
	{
		if (AssociatedNode.IsValid())
		{
			if (const UEnum* Enum = AssociatedNode->GetEnum())
			{
				return FEnumViewerUtils::GenerateContextMenuWidget(Enum);
			}
		}

		return SNullWidget::NullWidget;
	}
}
