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
#include "Widgets/Views/STreeView.h"
#include "EnumViewer/EnumViewerSettings.h"
#include "Engine/EngineTypes.h"

class FMenuBuilder;
class FTextFilterExpressionEvaluator;
class UBlueprint;
class SComboButton;
class FEnumViewerNode;
class FTextFilterExpressionEvaluator;

namespace EnumViewer
{
	class SEnumViewer : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SEnumViewer)
		{
		}

		SLATE_ARGUMENT(FOnEnumPicked, OnEnumPickedDelegate)

		SLATE_END_ARGS()

		/**
		 * Conenum the widget
		 *
		 * @param	InArgs			A declaration from which to conenum the widget
		 * @param	InitOptions		Programmer-driven initialization options for this widget
		 */
		void Conenum(const FArguments& InArgs, const FEnumViewerInitializationOptions& InInitOptions);

		/** Gets the widget contents of the app */
		virtual TSharedRef<SWidget> GetContent();

		virtual ~SEnumViewer() override;

		/** Empty the selection set. */
		virtual void ClearSelection();

		/** SWidget interface */
		virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
		virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
		virtual FReply OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& InFocusEvent) override;
		virtual bool SupportsKeyboardFocus() const override;

		/** Test to see whether the given enum would be allowed by this enum viewer */
		virtual bool IsEnumAllowed(const UEnum* InEnum) const;

		/** Destroys the internal Enum Hierarchy database */
		static void DestroyEnumHierarchy();

	private:
		/** Retrieves the children for the input node.
		 *	@param InParent				The parent node to retrieve the children from.
		 *	@param OutChildren			List of children for the parent node.
		 *
		 */
		void OnGetChildrenForEnumViewerTree(TSharedPtr<FEnumViewerNode> InParent, TArray<TSharedPtr<FEnumViewerNode>>& OutChildren);

		/** Creates the row widget when called by Slate when an item appears on the tree. */
		TSharedRef<ITableRow> OnGenerateRowForEnumViewer(TSharedPtr<FEnumViewerNode> Item, const TSharedRef<STableViewBase>& OwnerTable);

		/** Invoked when the user attempts to drag an item out of the enum browser */
		FReply OnDragDetected(const FGeometry& Geometry, const FPointerEvent& PointerEvent) override;

		/** Called by Slate when the filter box changes text. */
		void OnFilterTextChanged(const FText& InFilterText);

		/** Called when enter is hit in search box */
		void OnFilterTextCommitted(const FText& InText, ETextCommit::Type CommitInfo);

		/** Called by Slate when an item is selected from the tree/list. */
		void OnEnumViewerSelectionChanged(TSharedPtr<FEnumViewerNode> Item, ESelectInfo::Type SelectInfo);

		/** Called by Slate when an item is expanded/collapsed from the tree/list. */
		void OnEnumViewerExpansionChanged(TSharedPtr<FEnumViewerNode> Item, bool bExpanded);

		/**
		 *	Sets all expansion states in the tree.
		 *
		 *	@param bInExpansionState			The expansion state to set the tree to.
		 */
		void SetAllExpansionStates(bool bInExpansionState);

		/**
		 *	A helper function to recursively set the tree.
		 *
		 *	@param	InNode						The current node in the tree.
		 *	@param	bInExpansionState			The expansion state to set the tree to.
		 */
		void SetAllExpansionStates_Helper(TSharedPtr<FEnumViewerNode> InNode, bool bInExpansionState);

		/**
		 *	A helper function to toggle expansion state of a single node
		 *
		 *	@param	InNode						The node to toggle expansion.
		 */
		void ToggleExpansionState_Helper(TSharedPtr<FEnumViewerNode> InNode);

		/** Builds the right click menu widget for the selected node. */
		TSharedPtr<SWidget> BuildMenuWidget();

		/** Recursive function to expand nodes not filtered out of the tree
		*	@param	InNode				The current node to inspect for expansion.
		*
		*	@return bool				true if the child expanded, thus the parent should.
		*/
		bool ExpandFilteredInNodes(TSharedPtr<FEnumViewerNode> InNode);

		/** Recursive function to map the expansion states of items in the tree.
		 *	@param InItem		The current item to examine the expansion state of.
		 */
		void MapExpansionStatesInTree(TSharedPtr<FEnumViewerNode> InItem);

		/** Recursive function to set the expansion states of items in the tree.
		 *	@param InItem		The current item to set the expansion state of.
		 */
		void SetExpansionStatesInTree(TSharedPtr<FEnumViewerNode> InItem);

		/** Sends a requests to the Enum Viewer to refresh itself the next chance it gets */
		void Refresh();

		/** Populates the tree with items based on the current filter. */
		void Populate();

		/** Returns an array of the currently selected EnumViewerNodes */
		const TArray<TSharedPtr<FEnumViewerNode>> GetSelectedItems() const;

		/** Expands all of the root nodes */
		virtual void ExpandRootNodes();

		/** Returns the foreground color for the view button */
		FSlateColor GetViewButtonForegroundColor() const;

		/** Handler for when the view combo button is clicked */
		TSharedRef<SWidget> GetViewButtonContent();

		/** Gets the text for the enum count label */
		FText GetEnumCountText() const;

		/** Sets the view type and updates lists accordingly */
		void SetCurrentDeveloperViewType(EEnumViewerDeveloperType NewType);

		/** Gets the current view type (list or tile) */
		EEnumViewerDeveloperType GetCurrentDeveloperViewType() const;

		/** Returns true if ViewType is the current view type */
		bool IsCurrentDeveloperViewType(EEnumViewerDeveloperType ViewType) const;

		/** Toggle whether internal use enums should be shown or not */
		void ToggleShowInternalEnums();

		/** Whether or not it's possible to show internal use enums */
		bool IsShowingInternalEnums() const;

		/** Whether or not it's possible to show internal use enums */
		bool IsToggleShowInternalEnumsAllowed() const;

		/** Get the total number of enums passing the current filters.*/
		const int GetNumItems() const;

		/** Count the number of tree items in the specified hierarchy*/
		int32 CountTreeItems(FEnumViewerNode* Node);

		/** Handle the settings for EnumViewer changing.*/
		void HandleSettingChanged(FName PropertyName);

		/** Accessor for the enum names that have been marked as internal only in settings */
		void GetInternalOnlyEnums(TArray<TSoftObjectPtr<const UEnum>>& Enums);

		/** Accessor for the enum paths that have been marked as internal only in settings */
		void GetInternalOnlyPaths(TArray<FDirectoryPath>& Paths);

	private:
		/** Init options, cached */
		FEnumViewerInitializationOptions InitOptions;

		/** The items to be displayed in the tree. */
		TArray<TSharedPtr<FEnumViewerNode>> RootTreeItems;

		/** Compiled filter search terms. */
		TSharedPtr<FTextFilterExpressionEvaluator> TextFilterPtr;
		
		/** Holds the Slate List widget which holds the enums for the Enum Viewer. */
		TSharedPtr<SListView<TSharedPtr<FEnumViewerNode>>> EnumList;

		/** The enum Search Box, used for filtering the enums visible. */
		TSharedPtr<SSearchBox> SearchBox;

		/** true to filter for unloaded enums. */
		bool bShowUnloadedEnums;

		/** true to allow enum dynamic loading. */
		bool bEnableEnumDynamicLoading;

		/** Callback that's fired when a enum is selected while in 'enum picking' mode */
		FOnEnumPicked OnEnumPicked;

		/** true if expansions states should be saved when compiling. */
		bool bSaveExpansionStates;

		/** The map holding the expansion state map for the tree. */
		TMap<FName, bool> ExpansionStateMap;

		/** True if the Enum Viewer needs to be repopulated at the next appropriate opportunity, occurs whenever enums are added, removed, renamed, etc. */
		bool bNeedsRefresh;

		/** True if the search box will take keyboard focus next frame */
		bool bPendingFocusNextFrame;

		/** True if we need to set the tree expansion states according to our local copy next tick */
		bool bPendingSetExpansionStates;

		/** Indicates if the 'Show Internal Enums' option should be enabled or disabled */
		bool bCanShowInternalEnums;

		/** The button that displays view options */
		TSharedPtr<SComboButton> ViewOptionsComboButton;

		/** Number of enums that passed the filter*/
		int32 NumEnums;
	};
}
