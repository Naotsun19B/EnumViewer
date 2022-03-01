// Copyright 2022 Naotsun. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Views/STableRow.h"

namespace EnumViewer
{
	class FEnumViewerNode;
	
	/**
	 * Widget class in the enum viewer row.
	 */
	class SEnumViewerRow : public STableRow<TSharedPtr<FString>>
	{
	public:
		// Defines an event to be called when the enum line indicated by this widget is double-clicked.
		DECLARE_DELEGATE_OneParam(FOnDoubleCliced, TSharedPtr<FEnumViewerNode>);
		
	public:
		SLATE_BEGIN_ARGS(SEnumViewerRow)
			: _EnumDisplayName(FText::GetEmpty())
			, _bIsInEnumViewer(true)
			, _bDynamicEnumLoading(true)
			, _HighlightText(FText::GetEmpty())
			, _TextColor(FLinearColor::White)
		{
		}

		// The enum name this item contains.
		SLATE_ARGUMENT(FText, EnumDisplayName)
		
		// Whether the owner's enum viewer is in browser mode.
		SLATE_ARGUMENT(bool, bIsInEnumViewer)
		
		// Whether dynamic enum loading is allowed.
		SLATE_ARGUMENT(bool, bDynamicEnumLoading)
		
		/** The text this item should highlight, if any. */
		SLATE_ARGUMENT(FText, HighlightText)
		
		// The text color for this item.
		SLATE_ARGUMENT(FSlateColor, TextColor)
		
		// The enum viewer node with which this item is associated.
		SLATE_ARGUMENT(TSharedPtr<FEnumViewerNode>, AssociatedNode)
		
		// Called when the enum line indicated by this widget is double-clicked.
		SLATE_ARGUMENT(FOnDoubleCliced, OnDoubleClicked)
		
		// Called when this row is dragged.
		SLATE_EVENT(FOnDragDetected, OnDragDetected)

		SLATE_END_ARGS()

		// Constructor.
		void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView);

	private:
		// SWidget interface.
		virtual FReply OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent) override;
		// End of SWidget interface.

		// Returns the text color for the item based on if it is selected or not.
		FSlateColor GetTextColor() const;
		
		// Returns a tooltip for the enum in this row.
		TSharedPtr<IToolTip> GetTextTooltip() const;
		
		// Returns the visibility of the optional combo button.
		EVisibility GetOptionsVisibility() const;

		// Generates the drop down menu of options for this enum.
		TSharedRef<SWidget> GenerateOptionsMenu() const;
		
	private:
		// The name of the enum to which this item is associated.
		FText EnumDisplayName;

		// Whether the owner's enum viewer is in browser mode.
		bool bIsInEnumBrowser = false;

		// Whether dynamic enum loading is allowed.
		bool bDynamicEnumLoading = false;

		// The text color for this item.
		FSlateColor TextColor;

		// The enum viewer node with which this item is associated.
		TSharedPtr<FEnumViewerNode> AssociatedNode;
		
		// The event called when the enum line indicated by this widget is double-clicked.
		FOnDoubleCliced OnDoubleClicked;
	};
}
