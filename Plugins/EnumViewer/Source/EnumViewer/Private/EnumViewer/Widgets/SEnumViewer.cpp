// Copyright 2022 Naotsun. All Rights Reserved.

#include "EnumViewer/Widgets/SEnumViewer.h"
#include "EnumViewer/EnumViewerGlobals.h"
#include "EnumViewer/Utilities/EnumViewerSettings.h"
#include "EnumViewer/Utilities/EnumViewerProjectSettings.h"
#include "EnumViewer/Utilities/EnumViewerUtils.h"
#include "EnumViewer/Data/EnumRegistry.h"
#include "EnumViewer/Data/EnumViewerNode.h"
#include "EnumViewer/Widgets/SEnumViewerRow.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SScrollBorder.h"
#include "SListViewSelectorDropdownMenu.h"
#include "Misc/TextFilterExpressionEvaluator.h"
#include "EditorWidgetsModule.h"
#include "AssetRegistryModule.h"

#if BEFORE_UE_4_25
#include "DragAndDrop/AssetDragDropOp.h"
#else
#include "ContentBrowserDataDragDropOp.h"
#endif

#define LOCTEXT_NAMESPACE "EnumViewer"

namespace EnumViewer
{
#if BEFORE_UE_4_25
	namespace TextFilter
	{
		/**
		 * Before UE 4.25, there is no FBasicStringFilterExpressionContext,
		 * so define your own custom text filter.
		 */
		class FEnumFilterContext : public ITextFilterExpressionContext
		{
		public:
			// Constructor.
			explicit FEnumFilterContext(const FString& InStr)
				: StrPtr(&InStr)
			{
			}

			// ITextFilterExpressionContext interface.
			virtual bool TestBasicStringExpression(
				const FTextFilterString& InValue,
				const ETextFilterTextComparisonMode InTextComparisonMode
			) const override
			{
				return TextFilterUtils::TestBasicStringExpression(*StrPtr, InValue, InTextComparisonMode);
			}
			virtual bool TestComplexExpression(
				const FName& InKey,
				const FTextFilterString& InValue,
				const ETextFilterComparisonOperation InComparisonOperation,
				const ETextFilterTextComparisonMode InTextComparisonMode
			) const override
			{
				return false;
			}
			// End of ITextFilterExpressionContext interface.

		private:
			const FString* StrPtr;
		};
	}
	using FBasicStringFilterExpressionContext = TextFilter::FEnumFilterContext;
#endif
	
	void SEnumViewer::Construct(const FArguments& InArgs, const FEnumViewerInitializationOptions& InInitOptions)
	{
		InitOptions = InInitOptions;
		OnEnumPicked = InArgs._OnEnumPicked;
		TextFilterPtr = MakeShared<FTextFilterExpressionEvaluator>(ETextFilterExpressionEvaluatorMode::BasicString);

		// Bind the event when it needs to be refreshed.
		FEnumRegistry::Get().GetOnPopulateEnumViewer().AddSP(this, &SEnumViewer::Refresh);
		UEnumViewerSettings::OnSettingChanged().AddSP(this, &SEnumViewer::Refresh);

		// Create the asset discovery indicator.
		FEditorWidgetsModule& EditorWidgetsModule = FModuleManager::LoadModuleChecked<FEditorWidgetsModule>(TEXT("EditorWidgets"));
		const TSharedRef<SWidget> AssetDiscoveryIndicator = EditorWidgetsModule.CreateAssetDiscoveryIndicator(EAssetDiscoveryIndicatorScaleMode::Scale_Vertical);
		FOnContextMenuOpening OnContextMenuOpening;
		if (InitOptions.Mode == EEnumViewerMode::EnumBrowsing)
		{
			OnContextMenuOpening = FOnContextMenuOpening::CreateSP(this, &SEnumViewer::HandleOnContextMenuOpening);
		}

		SAssignNew(ListView, SListView<TSharedPtr<FEnumViewerNode>>)
			.SelectionMode(ESelectionMode::Single)
			.ListItemsSource(&EnumNodes)
			.OnGenerateRow(this, &SEnumViewer::HandleOnGenerateRow)
			.OnContextMenuOpening(OnContextMenuOpening)
			.OnSelectionChanged(this, &SEnumViewer::HandleOnSelectionChanged)
			.ItemHeight(20.0f)
			.HeaderRow
			(
				SNew(SHeaderRow)
				.Visibility(EVisibility::Collapsed)
				+SHeaderRow::Column(TEXT("Enum"))
				.DefaultLabel(NSLOCTEXT("EnumViewer", "Enum", "Enum"))
			);

		const TSharedPtr<SWidget> EnumViewerContent =
			SNew(SBox)
			.MaxDesiredHeight(800.0f)
			[
				SNew(SBorder)
				.BorderImage(FEditorStyle::GetBrush(InitOptions.bShowBackgroundBorder ? TEXT("ToolPanel.GroupBorder") : TEXT("NoBorder")))
				[
					SNew(SVerticalBox)
					// Title text.
					+SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SHorizontalBox)
						+SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Visibility(InitOptions.ViewerTitleString.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible)
							.ColorAndOpacity(FEditorStyle::GetColor(TEXT("MultiboxHookColor")))
							.Text(InitOptions.ViewerTitleString)
						]
					]
					// Search box.
					+SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SHorizontalBox)
						+SHorizontalBox::Slot()
						.Padding(2.0f, 2.0f)
						[
							SAssignNew(SearchBox, SSearchBox)
							.OnTextChanged(this, &SEnumViewer::HandleOnFilterTextChanged)
							.OnTextCommitted(this, &SEnumViewer::HandleOnFilterTextCommitted)
						]
					]
					// Separator (Browser mode only).
					+SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SSeparator)
						.Visibility((InitOptions.Mode == EEnumViewerMode::EnumBrowsing) ? EVisibility::Visible : EVisibility::Collapsed)
					]
					// Enum list view.
					+SVerticalBox::Slot()
					.FillHeight(1.0f)
					[
						SNew(SOverlay)
						+SOverlay::Slot()
						.HAlign(HAlign_Fill)
						.VAlign(VAlign_Fill)
						[
							SNew(SVerticalBox)
							
							+SVerticalBox::Slot()
							.FillHeight(1.0f)
							[
								SNew(SScrollBorder, ListView.ToSharedRef())
								[
									ListView.ToSharedRef()
								]
							]
						]
						// Asset discovery indicator
						+SOverlay::Slot()
						.HAlign(HAlign_Fill)
						.VAlign(VAlign_Bottom)
						.Padding(FMargin(24, 0, 24, 0))
						[
							AssetDiscoveryIndicator
						]
					]
					// Bottom panel.
					+SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SHorizontalBox)
						// Asset count.
						+SHorizontalBox::Slot()
						.FillWidth(1.f)
						.VAlign(VAlign_Center)
						.Padding(8, 0)
						[
							SNew(STextBlock)
							.Text(this, &SEnumViewer::HandleGetEnumCountText)
						]
						// View options combo button.
						+SHorizontalBox::Slot()
						.AutoWidth()
						[
							SAssignNew(ViewOptionsComboButton, SComboButton)
							.ContentPadding(0)
							.ForegroundColor(this, &SEnumViewer::HandleGetViewButtonForegroundColor)
							.ButtonStyle(FEditorStyle::Get(), TEXT("ToggleButton"))
							.OnGetMenuContent(this, &SEnumViewer::HandleGetViewButtonContent)
							.Visibility(InitOptions.bAllowViewOptions ? EVisibility::Visible : EVisibility::Collapsed)
							.ButtonContent()
							[
								SNew(SHorizontalBox)
								+SHorizontalBox::Slot()
								.AutoWidth()
								.VAlign(VAlign_Center)
								[
									SNew(SImage)
									.Image(FEditorStyle::GetBrush(TEXT("GenericViewButton")))
								]

								+SHorizontalBox::Slot()
								.AutoWidth()
								.Padding(2, 0, 0, 0)
								.VAlign(VAlign_Center)
								[
									SNew(STextBlock)
									.Text(LOCTEXT("ViewButton", "View Options"))
								]
							]
						]
					]
				]
			];

		// When using a enum picker in list-view mode, the widget will auto-focus the search box
		// and allow the up and down arrow keys to navigate and enter to pick without using the mouse ever.
		if (InitOptions.Mode == EEnumViewerMode::EnumPicker)
		{
			ChildSlot
			[
				SNew(SListViewSelectorDropdownMenu<TSharedPtr<FEnumViewerNode>>, SearchBox, ListView)
				[
					EnumViewerContent.ToSharedRef()
				]
			];
		}
		else
		{
			ChildSlot
			[
				EnumViewerContent.ToSharedRef()
			];
		}
		
		bNeedsRefresh = true;
		bPendingFocusNextFrame = true;
	}

	SEnumViewer::~SEnumViewer()
	{
		if (auto* EnumRegistry = FEnumRegistry::GetPtr())
		{
			EnumRegistry->GetOnPopulateEnumViewer().RemoveAll(this);
		}
		
		UEnumViewerSettings::OnSettingChanged().RemoveAll(this);
	}

	void SEnumViewer::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
	{
		SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
		
		// Move focus to search box
		if (bPendingFocusNextFrame && SearchBox.IsValid())
		{
			FWidgetPath WidgetToFocusPath;
			FSlateApplication::Get().GeneratePathToWidgetUnchecked(SearchBox.ToSharedRef(), WidgetToFocusPath);
			FSlateApplication::Get().SetKeyboardFocus(WidgetToFocusPath, EFocusCause::SetDirectly);
			bPendingFocusNextFrame = false;
		}

		if (bNeedsRefresh)
		{
			bNeedsRefresh = false;
			Populate();
		}
	}

	FReply SEnumViewer::OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& InFocusEvent)
	{
		FSlateApplication::Get().SetKeyboardFocus(SearchBox.ToSharedRef(), EFocusCause::SetDirectly);

		return FReply::Unhandled();
	}

	bool SEnumViewer::SupportsKeyboardFocus() const
	{
		return true;
	}

	FReply SEnumViewer::OnDragDetected(const FGeometry& Geometry, const FPointerEvent& PointerEvent)
	{
		if (InitOptions.Mode == EEnumViewerMode::EnumBrowsing)
		{
			const TArray<TSharedPtr<FEnumViewerNode>> SelectedItems = GetSelectedItems();
			if (SelectedItems.IsValidIndex(0))
			{
				const TSharedPtr<FEnumViewerNode> SelectedItem = SelectedItems[0];
				if (SelectedItem.IsValid())
				{
					// Spawn a loaded user defined enum just like any other asset from the Content Browser.
					const FAssetData AssetData = FAssetRegistryModule::GetRegistry().GetAssetByObjectPath(SelectedItem->GetEnumPath());
					if (AssetData.IsValid())
					{
#if BEFORE_UE_4_25
						return FReply::Handled().BeginDragDrop(FAssetDragDropOp::New(AssetData));
#else
						return FReply::Handled().BeginDragDrop(FContentBrowserDataDragDropOp::Legacy_New(MakeArrayView(&AssetData, 1)));
#endif
					}
				}
			}
		}

		return FReply::Unhandled();
	}

	TArray<TSharedPtr<FEnumViewerNode>> SEnumViewer::GetSelectedItems() const
	{
		if (ListView.IsValid())
		{
			return ListView->GetSelectedItems();
		}

		return {};
	}

	void SEnumViewer::Refresh()
	{
		bNeedsRefresh = true;
	}

	void SEnumViewer::Populate()
	{
		EnumNodes.Empty();

		const bool bShowingInternalEnums = IsShowingInternalEnums();
		TArray<FDirectoryPath> InternalPaths;
		
		// If we aren't showing the internal enums, then we need to know what enums to consider Internal Only, so let's gather them up from the settings object.
		if (!bShowingInternalEnums)
		{
			InternalPaths = GetInternalOnlyPaths();
			TArray<TSoftObjectPtr<const UEnum>> InternalEnumNames = GetInternalOnlyEnums();

			// Take the package names for the internal only enums and convert them into their UEnums
			for (const TSoftObjectPtr<const UEnum>& InternalEnumName : InternalEnumNames)
			{
				const TSharedPtr<FEnumViewerNode> EnumNode = FEnumRegistry::Get().FindNodeByEnumPath(*InternalEnumName.ToString());
				if (EnumNode.IsValid())
				{
					FDirectoryPath DirectoryPath;
					DirectoryPath.Path = EnumNode->GetEnumPath().ToString();
					InternalPaths.Add(DirectoryPath);
				}
			}
		}

		auto FilterPredicate = [&](const TSharedPtr<FEnumViewerNode>& EnumViewerNode) -> bool
		{
			const FString DeveloperPathWithSlash = FPackageName::FilenameToLongPackageName(FPaths::GameDevelopersDir());
			const FString UserDeveloperPathWithSlash = FPackageName::FilenameToLongPackageName(FPaths::GameUserDeveloperDir());
			const FString EnumPathString = EnumViewerNode->GetEnumPath().ToString();
			
			// Whether you cleared the filter for developer folders.
			bool bPassesDeveloperFilter = true;
			{
				const EEnumViewerDeveloperType AllowedDeveloperType = GetCurrentDeveloperViewType();
				if (AllowedDeveloperType == EEnumViewerDeveloperType::None)
				{
					bPassesDeveloperFilter = !EnumPathString.StartsWith(DeveloperPathWithSlash);
				}
				else if (AllowedDeveloperType == EEnumViewerDeveloperType::CurrentUser)
				{
					if (EnumPathString.StartsWith(DeveloperPathWithSlash))
					{
						bPassesDeveloperFilter = EnumPathString.StartsWith(UserDeveloperPathWithSlash);
					}
				}
			}
			
			// Whether the filter for internal use only has been cleared.
			bool bPassesInternalFilter = true;
			if (!bShowingInternalEnums && InternalPaths.Num() > 0)
			{
				for (const FDirectoryPath& InternalPath : InternalPaths)
				{
					if (EnumPathString.StartsWith(InternalPath.Path))
					{
						bPassesInternalFilter = false;
						break;
					}
				}
			}

			// Whether you cleared the text filter.
			bool bPassedTextFilter = true;
			if (TextFilterPtr.IsValid())
			{
				bPassedTextFilter = TextFilterPtr->TestTextFilter(
					FBasicStringFilterExpressionContext(EnumViewerNode->GetEnumName())
				);
			}

			// Whether the user-extensible filter has been cleared.
			bool bPassedCustomFilter = false;
			if (EnumViewerNode.IsValid())
			{
				bPassedCustomFilter = FEnumViewerUtils::IsEnumAllowed(InitOptions, EnumViewerNode->GetEnum());
			}
			else
			{
				if (InitOptions.bShowUnloadedEnums)
				{
					bPassedCustomFilter = FEnumViewerUtils::IsUnloadedEnumAllowed(InitOptions, EnumViewerNode->GetEnumPath());
				}
			}

			return (
				bPassesDeveloperFilter &&
				bPassesInternalFilter &&
				bPassedTextFilter &&
				bPassedCustomFilter
			);
		};
		
		// Get the enum list, passing in certain filter options.
		EnumNodes = FEnumRegistry::Get().GetNodeList(InitOptions.PropertyHandle, FilterPredicate);
		
		// In picker mode, delete the ones that did not clear the filter.
		if (InitOptions.Mode == EEnumViewerMode::EnumPicker)
		{
			EnumNodes.RemoveAll(
				[](const TSharedPtr<FEnumViewerNode>& EnumViewerNode) -> bool
				{
					if (EnumViewerNode.IsValid())
					{
						return !EnumViewerNode->PassedFilter();
					}

					return true;
				}
			);
		}
		
		// Sort the list alphabetically.
		EnumNodes.Sort(
			[](const TSharedPtr<FEnumViewerNode>& Lhs, const TSharedPtr<FEnumViewerNode>& Rhs) -> bool
			{
				check(Lhs.IsValid() && Rhs.IsValid());
				
				return (Lhs->GetEnumName() < Rhs->GetEnumName());
			}
		);
		
		if (InitOptions.bShowNoneOption && InitOptions.Mode == EEnumViewerMode::EnumPicker)
		{
			EnumNodes.Insert(MakeShared<FEnumViewerNode>(), 0);
		}

		if (ListView.IsValid())
		{
			ListView->RequestListRefresh();
		}
	}

	bool SEnumViewer::IsShowingInternalEnums() const
	{
		if (!InitOptions.bAllowViewOptions)
		{
			return true;
		}
		
		return UEnumViewerSettings::Get().bDisplayInternalEnums;
	}
	
	EEnumViewerDeveloperType SEnumViewer::GetCurrentDeveloperViewType() const
	{
		if (!InitOptions.bAllowViewOptions)
		{
			return EEnumViewerDeveloperType::All;
		}

		return UEnumViewerSettings::Get().DeveloperFolderType;
	}

	TArray<TSoftObjectPtr<const UEnum>> SEnumViewer::GetInternalOnlyEnums() const
	{
		if (!InitOptions.bAllowViewOptions)
		{
			return {};
		}
		
		return UEnumViewerProjectSettings::Get().InternalOnlyEnums;
	}

	TArray<FDirectoryPath> SEnumViewer::GetInternalOnlyPaths() const
	{
		if (!InitOptions.bAllowViewOptions)
		{
			return {};
		}
		
		return UEnumViewerProjectSettings::Get().InternalOnlyPaths;
	}

	void SEnumViewer::ToggleShowInternalEnums()
	{
		const bool bCurrentState = UEnumViewerSettings::Get().bDisplayInternalEnums;
		FEnumViewerSettingsModifier::SetDisplayInternalEnums(!bCurrentState);
	}

	bool SEnumViewer::IsCurrentDeveloperViewType(EEnumViewerDeveloperType ViewType) const
	{
		return (GetCurrentDeveloperViewType() == ViewType);
	}

	void SEnumViewer::SetCurrentDeveloperViewType(EEnumViewerDeveloperType NewType)
	{
		if (ensure(NewType < EEnumViewerDeveloperType::Max) &&
			NewType != UEnumViewerSettings::Get().DeveloperFolderType)
		{
			FEnumViewerSettingsModifier::SetDeveloperFolderType(NewType);
		}
	}

	void SEnumViewer::HandleOnFilterTextChanged(const FText& InFilterText)
	{
		if (TextFilterPtr.IsValid())
		{
			TextFilterPtr->SetFilterText(InFilterText);

			if (SearchBox.IsValid())
			{
				SearchBox->SetError(TextFilterPtr->GetFilterErrorText());
			}
		}

		Refresh();
	}

	void SEnumViewer::HandleOnFilterTextCommitted(const FText& InText, ETextCommit::Type CommitInfo)
	{
		if (CommitInfo == ETextCommit::OnEnter)
		{
			if (ListView.IsValid() && InitOptions.Mode == EEnumViewerMode::EnumPicker)
			{
				TArray<TSharedPtr<FEnumViewerNode>> SelectedList = ListView->GetSelectedItems();
				if (SelectedList.IsValidIndex(0))
				{
					const TSharedPtr<FEnumViewerNode> FirstSelected = SelectedList[0];
					const UEnum* Enum = FirstSelected->GetEnum();

					// Try and ensure the enum is loaded
					if (InitOptions.bEnableEnumDynamicLoading && !IsValid(Enum))
					{
						FirstSelected->LoadEnum();
						Enum = FirstSelected->GetEnum();
					}

					// Check if the item passes the filter, parent items might be displayed but filtered out and thus not desired to be selected.
					if (Enum && FirstSelected->PassedFilter())
					{
						OnEnumPicked.ExecuteIfBound(Enum);
					}
				}
			}
		}
	}

	FText SEnumViewer::HandleGetEnumCountText() const
	{
		const int32 NumOfEnums = EnumNodes.Num();
		const int32 NumOfSelectedEnums = GetSelectedItems().Num();
		if (NumOfSelectedEnums == 0)
		{
			return FText::Format(LOCTEXT("EnumCountLabel", "{0} {0}|plural(one=item,other=items)"), NumOfEnums);
		}
		else
		{
			return FText::Format(LOCTEXT("EnumCountLabelPlusSelection", "{0} {0}|plural(one=item,other=items) ({1} selected)"), NumOfEnums, NumOfSelectedEnums);
		}
	}

	FSlateColor SEnumViewer::HandleGetViewButtonForegroundColor() const
	{
		return (
			ViewOptionsComboButton->IsHovered() ?
			FEditorStyle::GetSlateColor(TEXT("InvertedForeground")) :
			FEditorStyle::GetSlateColor(TEXT("DefaultForeground"))
		);
	}

	TSharedRef<SWidget> SEnumViewer::HandleGetViewButtonContent()
	{
		FMenuBuilder MenuBuilder(true, nullptr, nullptr, true);
		
		MenuBuilder.BeginSection(TEXT("Filters"), LOCTEXT("EnumViewerFiltersHeading", "Enum Filters"));
		{
			MenuBuilder.AddMenuEntry(
				LOCTEXT("ShowInternalEnumsOption", "Show Internal Enums"),
				LOCTEXT("ShowInternalEnumsOptionToolTip", "Shows internal-use only enums in the view."),
				FSlateIcon(),
				FUIAction(
					FExecuteAction::CreateSP(this, &SEnumViewer::ToggleShowInternalEnums),
					FCanExecuteAction(),
					FIsActionChecked::CreateSP(this, &SEnumViewer::IsShowingInternalEnums)
				),
				NAME_None,
				EUserInterfaceActionType::ToggleButton
			);
		}
		MenuBuilder.EndSection();

		MenuBuilder.BeginSection(TEXT("DeveloperViewType"), LOCTEXT("DeveloperViewTypeHeading", "Developer Folder Filter"));
		{
			MenuBuilder.AddMenuEntry(
				LOCTEXT("NoneDeveloperViewOption", "None"),
				LOCTEXT("NoneDeveloperViewOptionToolTip", "Filter enums to show no enums in developer folders."),
				FSlateIcon(),
				FUIAction(
					FExecuteAction::CreateSP(this, &SEnumViewer::SetCurrentDeveloperViewType, EEnumViewerDeveloperType::None),
					FCanExecuteAction(),
					FIsActionChecked::CreateSP(this, &SEnumViewer::IsCurrentDeveloperViewType, EEnumViewerDeveloperType::None)
				),
				NAME_None,
				EUserInterfaceActionType::RadioButton
			);

			MenuBuilder.AddMenuEntry(
				LOCTEXT("CurrentUserDeveloperViewOption", "Current Developer"),
				LOCTEXT("CurrentUserDeveloperViewOptionToolTip", "Filter enums to allow enums in the current user's development folder."),
				FSlateIcon(),
				FUIAction(
					FExecuteAction::CreateSP(this, &SEnumViewer::SetCurrentDeveloperViewType, EEnumViewerDeveloperType::CurrentUser),
					FCanExecuteAction(),
					FIsActionChecked::CreateSP(this, &SEnumViewer::IsCurrentDeveloperViewType, EEnumViewerDeveloperType::CurrentUser)
				),
				NAME_None,
				EUserInterfaceActionType::RadioButton
			);

			MenuBuilder.AddMenuEntry(
				LOCTEXT("AllUsersDeveloperViewOption", "All Developers"),
				LOCTEXT("AllUsersDeveloperViewOptionToolTip", "Filter enums to allow enums in all users' development folders."),
				FSlateIcon(),
				FUIAction(
					FExecuteAction::CreateSP(this, &SEnumViewer::SetCurrentDeveloperViewType, EEnumViewerDeveloperType::All),
					FCanExecuteAction(),
					FIsActionChecked::CreateSP(this, &SEnumViewer::IsCurrentDeveloperViewType, EEnumViewerDeveloperType::All)
				),
				NAME_None,
				EUserInterfaceActionType::RadioButton
			);
		}
		MenuBuilder.EndSection();

		return MenuBuilder.MakeWidget();
	}

	TSharedPtr<SWidget> SEnumViewer::HandleOnContextMenuOpening()
	{
		TArray<TSharedPtr<FEnumViewerNode>> SelectedItems = GetSelectedItems();
		if (SelectedItems.IsValidIndex(0))
		{
			const TSharedPtr<FEnumViewerNode> SelectedItem = SelectedItems[0];
			if (SelectedItem.IsValid())
			{
				const UEnum* SelectedEnum = SelectedItem->GetEnum();
				if (InitOptions.bEnableEnumDynamicLoading && !IsValid(SelectedEnum))
				{
					SelectedItem->LoadEnum();
					SelectedEnum = SelectedItem->GetEnum();

					Refresh();
				}
				
				return FEnumViewerUtils::GenerateContextMenuWidget(SelectedEnum);
			}
		}

		return SNullWidget::NullWidget;
	}

	TSharedRef<ITableRow> SEnumViewer::HandleOnGenerateRow(
		TSharedPtr<FEnumViewerNode> Item,
		const TSharedRef<STableViewBase>& OwnerTable
	)
	{
		return SNew(SEnumViewerRow, OwnerTable)
			.EnumDisplayName(Item->GetEnumDisplayName(InitOptions.NameTypeToDisplay))
			.HighlightText(SearchBox->GetText())
			.TextColor(FLinearColor(1.0f, 1.0f, 1.0f, (Item->PassedFilter() ? 1.0f : 0.5f)))
			.AssociatedNode(Item)
			.bIsInEnumViewer(InitOptions.Mode == EEnumViewerMode::EnumBrowsing)
			.bDynamicEnumLoading(InitOptions.bEnableEnumDynamicLoading)
			.OnDragDetected(this, &SEnumViewer::OnDragDetected);
	}

	void SEnumViewer::HandleOnSelectionChanged(TSharedPtr<FEnumViewerNode> Item, ESelectInfo::Type SelectInfo)
	{
		if (SelectInfo == ESelectInfo::OnNavigation)
		{
			return;
		}
		
		if (!Item.IsValid() || Item->IsRestricted())
		{
			return;
		}

		if (InitOptions.Mode == EEnumViewerMode::EnumPicker)
		{
			Item->LoadEnum();
			OnEnumPicked.ExecuteIfBound(Item->GetEnum());
		}
	}
}

#undef LOCTEXT_NAMESPACE
