// Copyright 2022 Naotsun. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "EnumViewer/IEnumViewer.h"
#include "EnumViewer/Types/EnumViewerInitializationOptions.h"

class FTextFilterExpressionEvaluator;
enum class EEnumViewerDeveloperType : uint8;

namespace EnumViewer
{
	class FEnumViewerNode;
	
	/**
	 * Widget class for enum browser and enum picker.
	 */
	class SEnumViewer : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SEnumViewer)
		{
		}

		// Called when a enum is selected while in 'enum picking' mode. 
		SLATE_EVENT(FOnEnumPicked, OnEnumPicked)

		SLATE_END_ARGS()

		// Constructor.
		void Construct(const FArguments& InArgs, const FEnumViewerInitializationOptions& InInitOptions);

		// Destructor.
		virtual ~SEnumViewer() override;

		// SWidget interface.
		virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
		virtual FReply OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& InFocusEvent) override;
		virtual bool SupportsKeyboardFocus() const override;
		virtual FReply OnDragDetected(const FGeometry& Geometry, const FPointerEvent& PointerEvent) override;
		// End of SWidget interface.

		// Returns an array of the currently selected EnumViewerNodes. 
		TArray<TSharedPtr<FEnumViewerNode>> GetSelectedItems() const;
		
	private:
		// Sends a requests to the Enum Viewer to refresh itself the next chance it gets. 
		void Refresh();

		// Populates the list with items based on the current filter. 
		void Populate();

		// Returns whether or not it's possible to show internal use enums. 
		bool IsShowingInternalEnums() const;
		
		// Returns the current view type.
		EEnumViewerDeveloperType GetCurrentDeveloperViewType() const;
		
		// Returns a list of enum names marked as internal only in the settings.
		TArray<TSoftObjectPtr<const UEnum>> GetInternalOnlyEnums() const;

		// Returns a list of enumerated paths marked as internal only in the settings.
		TArray<FDirectoryPath> GetInternalOnlyPaths() const;

		// Toggle whether internal use enums should be shown or not. 
		void ToggleShowInternalEnums();
		
		// Returns true if ViewType is the current view type. 
		bool IsCurrentDeveloperViewType(EEnumViewerDeveloperType ViewType) const;

		// Sets the view type and updates lists accordingly. 
		void SetCurrentDeveloperViewType(EEnumViewerDeveloperType NewType);
		
		// Called by Slate when the filter box changes text. 
		void HandleOnFilterTextChanged(const FText& InFilterText);

		// Called when enter is hit in search box. 
		void HandleOnFilterTextCommitted(const FText& InText, ETextCommit::Type CommitInfo);
		
		// Called when the text of an enumerated count label is constructed.
		FText HandleGetEnumCountText() const;

		// Called when building the foreground color of the display options combo button.
		FSlateColor HandleGetViewButtonForegroundColor() const;

		// Called when the display option combo button is opened.
		TSharedRef<SWidget> HandleGetViewButtonContent();
		
		// Called when the context menu for the selected row is opened.
		TSharedPtr<SWidget> HandleOnContextMenuOpening();
		
		// Called when creating a widget for a row in a list.
		TSharedRef<ITableRow> HandleOnGenerateRow(
			TSharedPtr<FEnumViewerNode> Item,
			const TSharedRef<STableViewBase>& OwnerTable
		);

		// Called by Slate when an item is selected from the tree/list. 
		void HandleOnSelectionChanged(TSharedPtr<FEnumViewerNode> Item, ESelectInfo::Type SelectInfo);

	private:
		// A cache of init options passed in the constructor. 
		FEnumViewerInitializationOptions InitOptions;
		
		// The event that's fired when a enum is selected while in 'enum picking' mode. 
		FOnEnumPicked OnEnumPicked;

		// Compiled filter search terms. 
		TSharedPtr<FTextFilterExpressionEvaluator> TextFilterPtr;

		// A list of enum nodes to display in this enum viewer.
		TArray<TSharedPtr<FEnumViewerNode>> EnumNodes;
		
		// An instance of the list view widget used within the enum viewer.
		TSharedPtr<SListView<TSharedPtr<FEnumViewerNode>>> ListView;

		// An instance of a search box widget for searching in a list of enums.
		TSharedPtr<SSearchBox> SearchBox;

		// An instance of the options combo button widget.
		TSharedPtr<SComboButton> ViewOptionsComboButton;

		// Whether the enum viewer needs to be reconfigured at the next appropriate time.
		// A refresh is required every time an enum is added, deleted, renamed, and so on.
		bool bNeedsRefresh = false;
		
		// Whether the search box gets keyboard focus in the next frame.
		bool bPendingFocusNextFrame = false;
	};
}
