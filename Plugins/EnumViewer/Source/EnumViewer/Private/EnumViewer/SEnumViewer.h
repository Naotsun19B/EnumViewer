// Copyright 2022 Naotsun. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SlateFwd.h"
#include "Input/Reply.h"
#include "EnumViewer/IEnumViewer.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SWidget.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STableViewBase.h"
#include "Widgets/Views/STableRow.h"
#include "EnumViewer/EnumViewerSettings.h"
#include "Engine/EngineTypes.h"

class FMenuBuilder;
class FTextFilterExpressionEvaluator;
class UBlueprint;
class SComboButton;
class FTextFilterExpressionEvaluator;

namespace EnumViewer
{
	class FEnumViewerNode;
	
	class SEnumViewer : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SEnumViewer)
		{
		}

		SLATE_ARGUMENT(FOnEnumPicked, OnEnumPickedDelegate)

		SLATE_END_ARGS()

		// Constructor.
		void Construct(const FArguments& InArgs, const FEnumViewerInitializationOptions& InInitOptions);

		// Gets the widget contents of the app.
		virtual TSharedRef<SWidget> GetContent();

		virtual ~SEnumViewer() override;
		
		// SWidget interface.
		virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
		virtual FReply OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& InFocusEvent) override;
		virtual bool SupportsKeyboardFocus() const override;
		virtual FReply OnDragDetected(const FGeometry& Geometry, const FPointerEvent& PointerEvent) override;
		// End of SWidget interface.
		
		// Destroys the internal Enum Hierarchy database. 
		static void DestroyEnumHierarchy();

	private:
		// Creates the row widget when called by Slate when an item appears on the tree. 
		TSharedRef<ITableRow> OnGenerateRowForEnumViewer(TSharedPtr<FEnumViewerNode> Item, const TSharedRef<STableViewBase>& OwnerTable);
		
		// Called by Slate when the filter box changes text. 
		void OnFilterTextChanged(const FText& InFilterText);

		// Called when enter is hit in search box. 
		void OnFilterTextCommitted(const FText& InText, ETextCommit::Type CommitInfo);

		// Called by Slate when an item is selected from the tree/list. 
		void OnEnumViewerSelectionChanged(TSharedPtr<FEnumViewerNode> Item, ESelectInfo::Type SelectInfo);
		
		// Builds the right click menu widget for the selected node. 
		TSharedPtr<SWidget> BuildMenuWidget();

		// Sends a requests to the Enum Viewer to refresh itself the next chance it gets. 
		void Refresh();

		// Populates the tree with items based on the current filter. 
		void Populate();

		// Returns an array of the currently selected EnumViewerNodes. 
		TArray<TSharedPtr<FEnumViewerNode>> GetSelectedItems() const;

		// Returns the foreground color for the view button. 
		FSlateColor GetViewButtonForegroundColor() const;

		// Handler for when the view combo button is clicked. 
		TSharedRef<SWidget> GetViewButtonContent();

		// Gets the text for the enum count label. 
		FText GetEnumCountText() const;

		// Sets the view type and updates lists accordingly. 
		void SetCurrentDeveloperViewType(EEnumViewerDeveloperType NewType);

		// Gets the current view type (list or tile).
		EEnumViewerDeveloperType GetCurrentDeveloperViewType() const;

		// Returns true if ViewType is the current view type. 
		bool IsCurrentDeveloperViewType(EEnumViewerDeveloperType ViewType) const;

		// Toggle whether internal use enums should be shown or not. 
		void ToggleShowInternalEnums();

		// Whether or not it's possible to show internal use enums. 
		bool IsShowingInternalEnums() const;

		// Whether or not it's possible to show internal use enums. 
		bool IsToggleShowInternalEnumsAllowed() const;

		// Get the total number of enums passing the current filters.
		int32 GetNumItems() const;

		// Handle the settings for EnumViewer changing.
		void HandleSettingChanged(FName PropertyName);

		// Accessor for the enum names that have been marked as internal only in settings. 
		void GetInternalOnlyEnums(TArray<TSoftObjectPtr<const UEnum>>& Enums);

		// Accessor for the enum paths that have been marked as internal only in settings. 
		void GetInternalOnlyPaths(TArray<FDirectoryPath>& Paths);

	private:
		// Init options, cached 
		FEnumViewerInitializationOptions InitOptions;
		
		// Compiled filter search terms. 
		TSharedPtr<FTextFilterExpressionEvaluator> TextFilterPtr;

		// The items to be displayed in the tree.
		TArray<TSharedPtr<FEnumViewerNode>> RootTreeItems;
		
		// Holds the Slate List widget which holds the enums for the Enum Viewer. 
		TSharedPtr<SListView<TSharedPtr<FEnumViewerNode>>> EnumList;

		// The enum Search Box, used for filtering the enums visible. 
		TSharedPtr<SSearchBox> SearchBox;

		// true to filter for unloaded enums. 
		bool bShowUnloadedEnums = false;

		// true to allow enum dynamic loading. 
		bool bEnableEnumDynamicLoading = false;

		// Callback that's fired when a enum is selected while in 'enum picking' mode. 
		FOnEnumPicked OnEnumPicked;
		
		// True if the Enum Viewer needs to be repopulated at the next appropriate opportunity, occurs whenever enums are added, removed, renamed, etc. 
		bool bNeedsRefresh = false;

		// True if the search box will take keyboard focus next frame. 
		bool bPendingFocusNextFrame = false;
		
		// Indicates if the 'Show Internal Enums' option should be enabled or disabled. 
		bool bCanShowInternalEnums = false;

		// The button that displays view options. 
		TSharedPtr<SComboButton> ViewOptionsComboButton;

		// Number of enums that passed the filter.
		int32 NumEnums = 0;
	};
}
