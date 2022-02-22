// Copyright 2022 Naotsun. All Rights Reserved.

#include "SEnumViewer.h"
#include "EnumViewer/EnumViewerGlobals.h"
#include "EnumViewer/EnumViewerNode.h"
#include "EnumViewer/EnumViewerFilter.h"
#include "EnumViewer/EnumViewerSettings.h"
#include "Misc/PackageName.h"
#include "Misc/ScopedSlowTask.h"
#include "Modules/ModuleManager.h"
#include "UObject/UObjectIterator.h"
#include "Misc/HotReloadInterface.h"
#include "Misc/TextFilterExpressionEvaluator.h"
#include "Editor.h"
#include "EditorStyleSet.h"
#include "SlateOptMacros.h"
#include "EditorWidgetsModule.h"
#include "Framework/Commands/UIAction.h"
#include "Framework/Commands/UICommandList.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SToolTip.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Layout/SScrollBorder.h"
#include "SListViewSelectorDropdownMenu.h"
#include "AssetRegistryModule.h"
#include "ContentBrowserDataDragDropOp.h"
#include "Editor/UnrealEdEngine.h"
#include "Engine/UserDefinedEnum.h"
#include "IDocumentation.h"
#include "PropertyHandle.h"
#include "Logging/LogMacros.h"
#include "SourceCodeNavigation.h"
#include "Subsystems/AssetEditorSubsystem.h"

#define LOCTEXT_NAMESPACE "SEnumViewer"

namespace EnumViewer
{
	class FEnumHierarchy
	{
	public:
		FEnumHierarchy();
		~FEnumHierarchy();

		/** Get the singleton instance, creating it if required */
		static FEnumHierarchy& Get();

		/** Get the singleton instance, or null if it doesn't exist */
		static FEnumHierarchy* GetPtr();

		/** Destroy the singleton instance */
		static void DestroyInstance();

		/** Used to inform any registered Enum Viewers to refresh. */
		DECLARE_MULTICAST_DELEGATE(FPopulateEnumViewer);
		FPopulateEnumViewer& GetPopulateEnumViewerDelegate()
		{
			return PopulateEnumViewerDelegate;
		}

		/** Get the nodes of the list */
		TArray<TSharedPtr<FEnumViewerNode>> GetNodeList(const TSharedPtr<IPropertyHandle>& InPropertyHandle, const bool bInPassedFilter) const
		{
			TArray<TSharedPtr<FEnumViewerNode>> Nodes;
			for (const auto& EnumNode : EnumNodes)
			{
				if (!EnumNode.IsValid())
				{
					continue;
				}

				Nodes.Add(MakeShared<FEnumViewerNode>(EnumNode.ToSharedRef(), InPropertyHandle, bInPassedFilter));
			}
			return Nodes;
		}
	
		/**
		 * Finds the node, recursively going deeper into the hierarchy.
		 */
		TSharedPtr<FEnumViewerNodeData> FindNodeByEnumPath(const FName InEnumPath)
		{
			const TSharedPtr<FEnumViewerNodeData>* FoundEnumPtr = EnumNodes.FindByPredicate(
				[&InEnumPath](const TSharedPtr<FEnumViewerNodeData>& EnumNodePtr)
				{
					if (EnumNodePtr.IsValid())
					{
						return (EnumNodePtr->GetEnumPath() == InEnumPath);
					}

					return false;
				}
			);

			if (FoundEnumPtr != nullptr)
			{
				return *FoundEnumPtr;
			}

			return nullptr;
		}

		/** Update the enum hierarchy if it is pending a refresh */
		void UpdateEnumHierarchy();

	private:
		/** Dirty the enum hierarchy so it will be rebuilt on the next call to UpdateEnumHierarchy */
		void DirtyEnumHierarchy();

		/** Populates the enum hierarchy tree, pulling all the loaded and unloaded enums into a master data tree */
		void PopulateEnumHierarchy();
		
		/** Called when hot reload has finished */
		void OnHotReload(bool bWasTriggeredAutomatically);

		/** Called when modules are loaded or unloaded */
		void OnModulesChanged(FName ModuleThatChanged, EModuleChangeReason ReasonForChange);

	private:
		/** The enum hierarchy singleton that manages the unfiltered enum tree for the Enum Viewer. */
		static TUniquePtr<FEnumHierarchy> Singleton;

		/** True if the enum hierarchy should be refreshed */
		bool bRefreshEnumHierarchy = false;

		/** Used to inform any registered Enum Viewers to refresh. */
		FPopulateEnumViewer PopulateEnumViewerDelegate;

		/** The dummy enum data node that is used as a root point for the Enum Viewer. */
		TArray<TSharedPtr<FEnumViewerNodeData>> EnumNodes;
	};

	TUniquePtr<FEnumHierarchy> FEnumHierarchy::Singleton;

	namespace Helpers
	{
		/**
		 * Checks if the enum is allowed under the init options of the enum viewer currently building it's tree/list.
		 * @param InInitOptions		The enum viewer's options, holds the AllowedEnums and DisallowedEnums.
		 * @param InEnum			The enum to test against.
		 */
		bool IsEnumAllowed(const FEnumViewerInitializationOptions& InInitOptions, const TWeakObjectPtr<const UEnum> InEnum)
		{
			if (InInitOptions.EnumFilter.IsValid())
			{
				return InInitOptions.EnumFilter->IsEnumAllowed(InInitOptions, InEnum.Get());
			}
			return true;
		}

		/**
		 * Checks if the unloaded enum is allowed under the init options of the enum viewer currently building it's tree/list.
		 * @param InInitOptions		The enum viewer's options, holds the AllowedEnums and DisallowedEnums.
		 * @param InEnumPath		The path name to test against.
		 */
		bool IsEnumAllowed_UnloadedEnum(const FEnumViewerInitializationOptions& InInitOptions, const FName InEnumPath)
		{
			if (InInitOptions.EnumFilter.IsValid())
			{
				return InInitOptions.EnumFilter->IsUnloadedEnumAllowed(InInitOptions, InEnumPath);
			}
			return true;
		}

		/**
		 * Checks if the TestString passes the filter.
		 * @param InTestString		The string to test against the filter.
		 * @param InTextFilter		Compiled text filter to apply.
		 * @return	true if it passes the filter.
		 */
		bool PassesFilter(const FString& InTestString, const FTextFilterExpressionEvaluator& InTextFilter)
		{
			return InTextFilter.TestTextFilter(FBasicStringFilterExpressionContext(InTestString));
		}

		/**
		 * Recursive function to build the list, filtering out nodes based on the InitOptions and filter search terms.
		 * @param InInitOptions						The enum viewer's options, holds the AllowedEnums and DisallowedEnums.
		 * @param InOutNodeList						The list to add all the nodes to.
		 * @param InOriginalRootNode				The original root node holding the data we produce a filtered node for.
		 * @param InTextFilter						Compiled text filter to apply.
		 * @param bInShowUnloadedEnums			Filter option to not remove unloaded enums due to enum filter options.
		 * @param InAllowedDeveloperType            Filter option for dealing with developer folders.
		 * @param bInInternalEnums                Filter option for showing internal enums.
		 * @param InternalEnums                   The enums that have been marked as Internal Only.
		 * @param InternalPaths                     The paths that have been marked Internal Only.
		 * @return Returns true if the child passed the filter.
		 */
		void AddChildren_List(
			const FEnumViewerInitializationOptions& InInitOptions, 
			TArray<TSharedPtr<FEnumViewerNode>>& InOutNodeList,
			const TSharedRef<FEnumViewerNodeData>& InOriginalRootNode, 
			const FTextFilterExpressionEvaluator& InTextFilter,
			const bool bInShowUnloadedEnums,
			const EEnumViewerDeveloperType InAllowedDeveloperType,
			const bool bInInternalEnums,
			const TArray<const UEnum*>& InternalEnums, 
			const TArray<FDirectoryPath>& InternalPaths
			)
		{
			static const FString DeveloperPathWithSlash = FPackageName::FilenameToLongPackageName(FPaths::GameDevelopersDir());
			static const FString UserDeveloperPathWithSlash = FPackageName::FilenameToLongPackageName(FPaths::GameUserDeveloperDir());

			const UEnum* OriginalRootNodeEnum = InOriginalRootNode->GetEnum();

			// Determine if we allow any developer folder enums, if so determine if this enum is in one of the allowed developer folders.
			const FString GeneratedEnumPathString = InOriginalRootNode->GetEnumPath().ToString();
			bool bPassesDeveloperFilter = true;
			if (InAllowedDeveloperType == EEnumViewerDeveloperType::None)
			{
				bPassesDeveloperFilter = !GeneratedEnumPathString.StartsWith(DeveloperPathWithSlash);
			}
			else if (InAllowedDeveloperType == EEnumViewerDeveloperType::CurrentUser)
			{
				if (GeneratedEnumPathString.StartsWith(DeveloperPathWithSlash))
				{
					bPassesDeveloperFilter = GeneratedEnumPathString.StartsWith(UserDeveloperPathWithSlash);
				}
			}

			// The INI files declare enums and folders that are considered internal only. Does this enum match any of those patterns?
			// INI path: /Script/EnumViewer.EnumViewerProjectSettings
			bool bPassesInternalFilter = true;
			if (!bInInternalEnums && InternalPaths.Num() > 0)
			{
				for (const FDirectoryPath& InternalPath : InternalPaths)
				{
					if (GeneratedEnumPathString.StartsWith(InternalPath.Path))
					{
						bPassesInternalFilter = false;
						break;
					}
				}
			}

			// There are few options for filtering an unloaded enum, if it matches with this filter, it passes.
			bool bPassedFilter = false;
			if (OriginalRootNodeEnum)
			{
				bPassedFilter = bPassesDeveloperFilter && bPassesInternalFilter && IsEnumAllowed(InInitOptions, OriginalRootNodeEnum) && PassesFilter(InOriginalRootNode->GetEnumName(), InTextFilter);
			}
			else
			{
				if (bInShowUnloadedEnums)
				{
					bPassedFilter = bPassesDeveloperFilter && bPassesInternalFilter && IsEnumAllowed_UnloadedEnum(InInitOptions, InOriginalRootNode->GetEnumPath()) && PassesFilter(InOriginalRootNode->GetEnumName(), InTextFilter);
				}
			}

			if (bPassedFilter)
			{
				InOutNodeList.Add(MakeShared<FEnumViewerNode>(InOriginalRootNode, InInitOptions.PropertyHandle, bPassedFilter));
			}
		}

		/**
		 * Opens an asset editor for a user defined enum.
		 */
		void OpenAssetEditor(const UUserDefinedEnum* InEnum)
		{
			if (InEnum)
			{
				GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(const_cast<UUserDefinedEnum*>(InEnum));
			}
		}

		/**
		 * Opens a enum source file.
		 */
		void OpenEnumInIDE(const UEnum* InEnum)
		{
			if (!InEnum)
			{
				return;
			}

			FString EnumHeaderPath;
			if (FSourceCodeNavigation::FindClassHeaderPath(InEnum, EnumHeaderPath) && IFileManager::Get().FileSize(*EnumHeaderPath) != INDEX_NONE)
			{
				const FString AbsoluteHeaderPath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*EnumHeaderPath);
				FSourceCodeNavigation::OpenSourceFile(AbsoluteHeaderPath);
			}
		}

		/**
		 * Finds the enum in the content browser.
		 */
		void FindInContentBrowser(const UEnum* InEnum)
		{
			if (InEnum)
			{
				TArray<UObject*> Objects = {
					const_cast<UEnum*>(InEnum)
				};
				GEditor->SyncBrowserToObjects(Objects);
			}
		}

		TSharedRef<SWidget> CreateMenu(const UEnum* InEnum)
		{
			// Empty list of commands.
			const TSharedPtr<FUICommandList> Commands;

			// Set the menu to automatically close when the user commits to a choice
			FMenuBuilder MenuBuilder(true, Commands);
			{
				if (const UUserDefinedEnum* UserDefinedEnum = Cast<UUserDefinedEnum>(InEnum))
				{
					MenuBuilder.AddMenuEntry(
						LOCTEXT("EnumViewerMenuEditEnumAsset", "Edit Enum..."), 
						LOCTEXT("EnumViewerMenuEditEnumAsset_Tooltip", "Open the enum in the asset editor."), 
						FSlateIcon(), 
						FUIAction(FExecuteAction::CreateStatic(&OpenAssetEditor, UserDefinedEnum))
						);

					MenuBuilder.AddMenuEntry(
						LOCTEXT("EnumViewerMenuFindContent", "Find in Content Browser..."), 
						LOCTEXT("EnumViewerMenuFindContent_Tooltip", "Find in Content Browser"), 
						FSlateIcon(), 
						FUIAction(FExecuteAction::CreateStatic(&FindInContentBrowser, InEnum))
						);
				}
				else
				{
					MenuBuilder.AddMenuEntry(
						LOCTEXT("EnumViewerMenuOpenSourceCode", "Open Source Code..."), 
						LOCTEXT("EnumViewerMenuOpenSourceCode_Tooltip", "Open the source file for this enum in the IDE."), 
						FSlateIcon(), 
						FUIAction(FExecuteAction::CreateStatic(&OpenEnumInIDE, InEnum))
						);
				}
			}

			return MenuBuilder.MakeWidget();
		}
	}

	/** Delegate used with the Enum Viewer in 'enum picking' mode. You'll bind a delegate when the enum viewer widget is created, which will be fired off when the selected enum is double clicked */
	DECLARE_DELEGATE_OneParam(FOnEnumItemDoubleClickDelegate, TSharedPtr<FEnumViewerNode>);

	/** The item used for visualizing the enum in the tree. */
	class SEnumItem : public STableRow<TSharedPtr<FString>>
	{
	public:

		SLATE_BEGIN_ARGS(SEnumItem)
			: _EnumDisplayName()
			, _bIsInEnumViewer(true)
			, _bDynamicEnumLoading(true)
			, _HighlightText()
			, _TextColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f))
		{}

		/** The enum name this item contains. */
		SLATE_ARGUMENT(FText, EnumDisplayName)
		/** true if this item is in a Enum Viewer (as opposed to a Enum Picker) */
		SLATE_ARGUMENT(bool, bIsInEnumViewer)
		/** true if this item should allow dynamic enum loading */
		SLATE_ARGUMENT(bool, bDynamicEnumLoading)
		/** The text this item should highlight, if any. */
		SLATE_ARGUMENT(FText, HighlightText)
		/** The color text this item will use. */
		SLATE_ARGUMENT(FSlateColor, TextColor)
		/** The node this item is associated with. */
		SLATE_ARGUMENT(TSharedPtr<FEnumViewerNode>, AssociatedNode)
		/** the delegate for handling double clicks outside of the SEnumItem */
		SLATE_ARGUMENT(FOnEnumItemDoubleClickDelegate, OnEnumItemDoubleClicked)
		/** On Enum Picked callback. */
		SLATE_EVENT(FOnDragDetected, OnDragDetected)

		SLATE_END_ARGS()

		/**
		 * Construct the widget
		 */
		void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
		{
			EnumDisplayName = InArgs._EnumDisplayName;
			bIsInEnumViewer = InArgs._bIsInEnumViewer;
			bDynamicEnumLoading = InArgs._bDynamicEnumLoading;
			AssociatedNode = InArgs._AssociatedNode;
			OnDoubleClicked = InArgs._OnEnumItemDoubleClicked;

			auto BuildToolTip = [this]() -> TSharedPtr<SToolTip>
			{
				TSharedPtr<SToolTip> ToolTip;

				const TSharedPtr<IPropertyHandle> PropertyHandle = AssociatedNode->GetPropertyHandle();
				if (PropertyHandle && AssociatedNode->IsRestricted())
				{
					FText RestrictionToolTip;
					PropertyHandle->GenerateRestrictionToolTip(*AssociatedNode->GetEnumName(), RestrictionToolTip);

					ToolTip = IDocumentation::Get()->CreateToolTip(RestrictionToolTip, nullptr, "", "");
				}
				else if (!AssociatedNode->GetEnumPath().IsNone())
				{
					ToolTip = SNew(SToolTip).Text(FText::FromName(AssociatedNode->GetEnumPath()));
				}

				return ToolTip;
			};

			const bool bIsRestricted = AssociatedNode->IsRestricted();

			this->ChildSlot
				[
					SNew(SHorizontalBox)

					+SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SExpanderArrow, SharedThis(this))
					]

					+SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.Padding(0.0f, 3.0f, 6.0f, 3.0f)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(EnumDisplayName)
						.HighlightText(InArgs._HighlightText)
						.ColorAndOpacity(this, &SEnumItem::GetTextColor)
						.ToolTip(BuildToolTip())
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
						.Visibility(this, &SEnumItem::ShowOptions)
						.OnGetMenuContent(this, &SEnumItem::GenerateDropDown)
					]
				];

			TextColor = InArgs._TextColor;

			UE_LOG(LogEnumViewer, VeryVerbose, TEXT("ENUM [%s]"), *EnumDisplayName.ToString());

			ConstructInternal(
				STableRow::FArguments()
				.ShowSelection(true)
				.OnDragDetected(InArgs._OnDragDetected),
				InOwnerTableView
			);
		}

	private:
		virtual FReply OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent) override
		{
			// If in a Enum Viewer and it has not been loaded, load the enum when double-left clicking.
			if (bIsInEnumViewer)
			{
				if (bDynamicEnumLoading && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
				{
					AssociatedNode->LoadEnum();
				}

				// If there is a enum asset, open its asset editor; otherwise try to open the enum header
				if (const UUserDefinedEnum* UserDefinedEnum = AssociatedNode->GetEnumAsset())
				{
					EnumViewer::Helpers::OpenAssetEditor(UserDefinedEnum);
				}
				else if (const UEnum* Enum = AssociatedNode->GetEnum())
				{
					EnumViewer::Helpers::OpenEnumInIDE(Enum);
				}
			}
			else
			{
				OnDoubleClicked.ExecuteIfBound(AssociatedNode);
			}

			return FReply::Handled();
		}

		EVisibility ShowOptions() const
		{
			// If it's in viewer mode, show the options combo button.
			return (bIsInEnumViewer && AssociatedNode->GetEnum())
				? EVisibility::Visible
				: EVisibility::Collapsed;
		}

		/**
		 * Generates the drop down menu for the item.
		 *
		 * @return		The drop down menu widget.
		 */
		TSharedRef<SWidget> GenerateDropDown()
		{
			if (const UEnum* Enum = AssociatedNode->GetEnum())
			{
				return EnumViewer::Helpers::CreateMenu(Enum);
			}

			return SNullWidget::NullWidget;
		}

		/** Returns the text color for the item based on if it is selected or not. */
		FSlateColor GetTextColor() const
		{
			const TSharedPtr<ITypedTableView<TSharedPtr<FString>>> OwnerWidget = OwnerTablePtr.Pin();
			const TSharedPtr<FString>* MyItem = OwnerWidget->Private_ItemFromWidget(this);
			const bool bIsSelected = OwnerWidget->Private_IsItemSelected(*MyItem);

			if (bIsSelected)
			{
				return FSlateColor::UseForeground();
			}

			return TextColor;
		}

	private:
		/** The enum name for which this item is associated with. */
		FText EnumDisplayName;

		/** true if in a Enum Viewer (as opposed to a Enum Picker). */
		bool bIsInEnumViewer = false;

		/** true if dynamic enum loading is permitted. */
		bool bDynamicEnumLoading = false;

		/** The text color for this item. */
		FSlateColor TextColor;

		/** The Enum Viewer Node this item is associated with. */
		TSharedPtr<FEnumViewerNode> AssociatedNode;

		/** the on Double Clicked delegate */
		FOnEnumItemDoubleClickDelegate OnDoubleClicked;
	};

	FEnumHierarchy::FEnumHierarchy()
	{
		// Register with the Asset Registry to be informed when it is done loading up files.
		const FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		AssetRegistryModule.Get().OnFilesLoaded().AddRaw(this, &FEnumHierarchy::DirtyEnumHierarchy);

		// Register to have Populate called when doing a Hot Reload.
		IHotReloadInterface& HotReloadSupport = FModuleManager::LoadModuleChecked<IHotReloadInterface>("HotReload");
		HotReloadSupport.OnHotReload().AddRaw(this, &FEnumHierarchy::OnHotReload);

		FModuleManager::Get().OnModulesChanged().AddRaw(this, &FEnumHierarchy::OnModulesChanged);

		PopulateEnumHierarchy();
	}

	FEnumHierarchy::~FEnumHierarchy()
	{
		// Unregister with the Asset Registry to be informed when it is done loading up files.
		if (FModuleManager::Get().IsModuleLoaded(TEXT("AssetRegistry")))
		{
			const FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
			AssetRegistryModule.Get().OnFilesLoaded().RemoveAll(this);
			AssetRegistryModule.Get().OnAssetAdded().RemoveAll(this);
			AssetRegistryModule.Get().OnAssetRemoved().RemoveAll(this);

			// Unregister to have Populate called when doing a Hot Reload.
			if (FModuleManager::Get().IsModuleLoaded("HotReload"))
			{
				IHotReloadInterface& HotReloadSupport = FModuleManager::GetModuleChecked<IHotReloadInterface>("HotReload");
				HotReloadSupport.OnHotReload().RemoveAll(this);
			}
		}

		FModuleManager::Get().OnModulesChanged().RemoveAll(this);
	}

	FEnumHierarchy& FEnumHierarchy::Get()
	{
		if (!Singleton)
		{
			Singleton = MakeUnique<FEnumHierarchy>();
			Singleton->PopulateEnumHierarchy();
		}
		return *Singleton;
	}

	FEnumHierarchy* FEnumHierarchy::GetPtr()
	{
		return Singleton.Get();
	}

	void FEnumHierarchy::DestroyInstance()
	{
		Singleton.Reset();
	}

	void FEnumHierarchy::UpdateEnumHierarchy()
	{
		if (bRefreshEnumHierarchy)
		{
			bRefreshEnumHierarchy = false;
			PopulateEnumHierarchy();
		}
	}

	void FEnumHierarchy::DirtyEnumHierarchy()
	{
		bRefreshEnumHierarchy = true;
	}

	void FEnumHierarchy::PopulateEnumHierarchy()
	{
		FScopedSlowTask SlowTask(0.0f, LOCTEXT("RebuildingEnumHierarchy", "Rebuilding Enum Hierarchy"));
		SlowTask.MakeDialog();

		auto AddUnique = [this](const TSharedPtr<FEnumViewerNodeData>& NewEnumViewerNodeData)
		{
			const auto* FoundEnumNode = EnumNodes.FindByPredicate(
				[&NewEnumViewerNodeData](const TSharedPtr<FEnumViewerNodeData>& EnumViewerNodeData) -> bool
				{
					if (EnumViewerNodeData.IsValid() && NewEnumViewerNodeData.IsValid())
					{
						return (EnumViewerNodeData->GetEnumPath() == NewEnumViewerNodeData->GetEnumPath());
					}

					return false;
				}
			);

			if (FoundEnumNode == nullptr)
			{
				EnumNodes.Add(NewEnumViewerNodeData);
			}
		};
		
		// Go through all of the enums and see if they should be added to the list.
		for (const auto* Enum : TObjectRange<UEnum>())
		{
			if (IsValid(Enum))
			{
				const TSharedPtr<FEnumViewerNodeData> EnumViewerNodeData = MakeShared<FEnumViewerNodeData>(Enum);
				AddUnique(EnumViewerNodeData);
			}
		}

		// Add any enum assets directly under the root (since they don't support inheritance)
		{
			const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

			FARFilter Filter;
			Filter.ClassNames.Add(UUserDefinedEnum::StaticClass()->GetFName());
			Filter.bRecursiveClasses = true;

			TArray<FAssetData> UserDefinedEnumsList;
			AssetRegistryModule.Get().GetAssets(Filter, UserDefinedEnumsList);
			
			for (const FAssetData& UserDefinedEnumData : UserDefinedEnumsList)
			{
				const TSharedPtr<FEnumViewerNodeData> EnumViewerNodeData = MakeShared<FEnumViewerNodeData>(UserDefinedEnumData);
				AddUnique(EnumViewerNodeData);
			}
		}
		
		// All viewers must refresh.
		PopulateEnumViewerDelegate.Broadcast();
	}
	
	void FEnumHierarchy::OnHotReload(bool bWasTriggeredAutomatically)
	{
		DirtyEnumHierarchy();
	}

	void FEnumHierarchy::OnModulesChanged(FName ModuleThatChanged, EModuleChangeReason ReasonForChange)
	{
		if (ReasonForChange == EModuleChangeReason::ModuleLoaded || ReasonForChange == EModuleChangeReason::ModuleUnloaded)
		{
			DirtyEnumHierarchy();
		}
	}

	BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
	void SEnumViewer::Construct(const FArguments& InArgs, const FEnumViewerInitializationOptions& InInitOptions)
	{
		bNeedsRefresh = true;
		NumEnums = 0;

		bCanShowInternalEnums = true;

		// Listen for when view settings are changed
		UEnumViewerSettings::OnSettingChanged().AddSP(this, &SEnumViewer::HandleSettingChanged);

		InitOptions = InInitOptions;

		OnEnumPicked = InArgs._OnEnumPickedDelegate;

		TextFilterPtr = MakeShareable(new FTextFilterExpressionEvaluator(ETextFilterExpressionEvaluatorMode::BasicString));

		bEnableEnumDynamicLoading = InInitOptions.bEnableEnumDynamicLoading;

		const EVisibility HeaderVisibility = (this->InitOptions.Mode == EEnumViewerMode::EnumBrowsing) ? EVisibility::Visible : EVisibility::Collapsed;

		// Set these values to the user specified settings.
		bShowUnloadedEnums = InitOptions.bShowUnloadedEnums;
		const bool bHasTitle = InitOptions.ViewerTitleString.IsEmpty() == false;

		// Create the asset discovery indicator
		FEditorWidgetsModule& EditorWidgetsModule = FModuleManager::LoadModuleChecked<FEditorWidgetsModule>("EditorWidgets");
		const TSharedRef<SWidget> AssetDiscoveryIndicator = EditorWidgetsModule.CreateAssetDiscoveryIndicator(EAssetDiscoveryIndicatorScaleMode::Scale_Vertical);
		FOnContextMenuOpening OnContextMenuOpening;
		if (InitOptions.Mode == EEnumViewerMode::EnumBrowsing)
		{
			OnContextMenuOpening = FOnContextMenuOpening::CreateSP(this, &SEnumViewer::BuildMenuWidget);
		}

		SAssignNew(EnumList, SListView<TSharedPtr<FEnumViewerNode>>)
			.SelectionMode(ESelectionMode::Single)
			.ListItemsSource(&RootTreeItems)
			// Generates the actual widget for a tree item
			.OnGenerateRow(this, &SEnumViewer::OnGenerateRowForEnumViewer)
			// Generates the right click menu.
			.OnContextMenuOpening(OnContextMenuOpening)
			// Find out when the user selects something in the tree
			.OnSelectionChanged(this, &SEnumViewer::OnEnumViewerSelectionChanged)
			// Allow for some spacing between items with a larger item height.
			.ItemHeight(20.0f)
			.HeaderRow
			(
				SNew(SHeaderRow)
				.Visibility(EVisibility::Collapsed)
				+SHeaderRow::Column(TEXT("Enum"))
				.DefaultLabel(NSLOCTEXT("EnumViewer", "Enum", "Enum"))
			);

		TSharedRef<SListView<TSharedPtr<FEnumViewerNode>>> EnumListView = EnumList.ToSharedRef();

		// Holds the bulk of the enum viewer's sub-widgets, to be added to the widget after construction
		const TSharedPtr<SWidget> EnumViewerContent =
			SNew(SBox)
			.MaxDesiredHeight(800.0f)
			[
				SNew(SBorder)
				.BorderImage(FEditorStyle::GetBrush(InitOptions.bShowBackgroundBorder ? "ToolPanel.GroupBorder" : "NoBorder"))
				[
					SNew(SVerticalBox)
					+SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SHorizontalBox)
						+SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Visibility(bHasTitle ? EVisibility::Visible : EVisibility::Collapsed)
							.ColorAndOpacity(FEditorStyle::GetColor("MultiboxHookColor"))
							.Text(InitOptions.ViewerTitleString)
						]
					]

					+SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SHorizontalBox)
						+SHorizontalBox::Slot()
						.Padding(2.0f, 2.0f)
						[
							SAssignNew(SearchBox, SSearchBox)
							.OnTextChanged(this, &SEnumViewer::OnFilterTextChanged)
							.OnTextCommitted(this, &SEnumViewer::OnFilterTextCommitted)
						]
					]

					+SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SSeparator)
						.Visibility(HeaderVisibility)
					]

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
								SNew(SScrollBorder, EnumListView)
								[
									EnumListView
								]
							]
						]

						+SOverlay::Slot()
						.HAlign(HAlign_Fill)
						.VAlign(VAlign_Bottom)
						.Padding(FMargin(24, 0, 24, 0))
						[
							// Asset discovery indicator
							AssetDiscoveryIndicator
						]
					]

					// Bottom panel
					+SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SHorizontalBox)

						// Asset count
						+SHorizontalBox::Slot()
						.FillWidth(1.f)
						.VAlign(VAlign_Center)
						.Padding(8, 0)
						[
							SNew(STextBlock)
							.Text(this, &SEnumViewer::GetEnumCountText)
						]

						// View mode combo button
						+SHorizontalBox::Slot()
						.AutoWidth()
						[
							SAssignNew(ViewOptionsComboButton, SComboButton)
							.ContentPadding(0)
							.ForegroundColor(this, &SEnumViewer::GetViewButtonForegroundColor)
							.ButtonStyle(FEditorStyle::Get(), "ToggleButton") // Use the tool bar item style for this button
							.OnGetMenuContent(this, &SEnumViewer::GetViewButtonContent)
							.ButtonContent()
							[
								SNew(SHorizontalBox)
								+SHorizontalBox::Slot()
								.AutoWidth()
								.VAlign(VAlign_Center)
								[
									SNew(SImage).Image(FEditorStyle::GetBrush("GenericViewButton"))
								]

								+SHorizontalBox::Slot()
								.AutoWidth()
								.Padding(2, 0, 0, 0)
								.VAlign(VAlign_Center)
								[
									SNew(STextBlock).Text(LOCTEXT("ViewButton", "View Options"))
								]
							]
						]
					]
				]
			];

		if (ViewOptionsComboButton.IsValid())
		{
			ViewOptionsComboButton->SetVisibility(InitOptions.bAllowViewOptions ? EVisibility::Visible : EVisibility::Collapsed);
		}

		// When using a enum picker in list-view mode, the widget will auto-focus the search box
		// and allow the up and down arrow keys to navigate and enter to pick without using the mouse ever
		if (InitOptions.Mode == EEnumViewerMode::EnumPicker)
		{
			this->ChildSlot
				[
					SNew(SListViewSelectorDropdownMenu<TSharedPtr<FEnumViewerNode>>, SearchBox, EnumList)
					[
						EnumViewerContent.ToSharedRef()
					]
				];
		}
		else
		{
			this->ChildSlot
				[
					EnumViewerContent.ToSharedRef()
				];
		}

		// Ensure the enum hierarchy exists, and and watch for changes
		FEnumHierarchy::Get().GetPopulateEnumViewerDelegate().AddSP(this, &SEnumViewer::Refresh);

		// Request delayed setting of focus to the search box
		bPendingFocusNextFrame = true;
	}
	END_SLATE_FUNCTION_BUILD_OPTIMIZATION

	TSharedRef<SWidget> SEnumViewer::GetContent()
	{
		return SharedThis(this);
	}

	SEnumViewer::~SEnumViewer()
	{
		// No longer watching for changes
		if (FEnumHierarchy* EnumHierarchy = FEnumHierarchy::GetPtr())
		{
			EnumHierarchy->GetPopulateEnumViewerDelegate().RemoveAll(this);
		}

		// Remove the listener for when view settings are changed
		UEnumViewerSettings::OnSettingChanged().RemoveAll(this);
	}

	void SEnumViewer::OnEnumViewerSelectionChanged(TSharedPtr<FEnumViewerNode> Item, ESelectInfo::Type SelectInfo)
	{
		// Do not act on selection change when it is for navigation
		if (SelectInfo == ESelectInfo::OnNavigation)
		{
			return;
		}

		// Sometimes the item is not valid anymore due to filtering.
		if (Item.IsValid() == false || Item->IsRestricted())
		{
			return;
		}

		if (InitOptions.Mode != EEnumViewerMode::EnumBrowsing)
		{
			// Attempt to ensure the enum is loaded
			Item->LoadEnum();

			// Check if the item passed the filter, parent items might be displayed but filtered out and thus not desired to be selected.
			if (Item->PassedFilter())
			{
				OnEnumPicked.ExecuteIfBound(Item->GetEnum());
			}
			else
			{
				OnEnumPicked.ExecuteIfBound(nullptr);
			}
		}
	}

	TSharedPtr<SWidget> SEnumViewer::BuildMenuWidget()
	{
		// Based upon which mode the viewer is in, pull the selected item.
		TArray<TSharedPtr<FEnumViewerNode>> SelectedList = EnumList->GetSelectedItems();

		// If there is no selected item, return a null widget.
		if (SelectedList.Num() == 0)
		{
			return SNullWidget::NullWidget;
		}

		// Get the enum
		const UEnum* RightClickEnum = SelectedList[0]->GetEnum();
		if (bEnableEnumDynamicLoading && !RightClickEnum)
		{
			SelectedList[0]->LoadEnum();
			RightClickEnum = SelectedList[0]->GetEnum();

			// Populate the tree/list so any changes to previously unloaded enums will be reflected.
			Refresh();
		}

		return EnumViewer::Helpers::CreateMenu(RightClickEnum);
	}

	TSharedRef<ITableRow> SEnumViewer::OnGenerateRowForEnumViewer(TSharedPtr<FEnumViewerNode> Item, const TSharedRef<STableViewBase>& OwnerTable)
	{
		// If the item was accepted by the filter, leave it bright, otherwise dim it.
		return SNew(SEnumItem, OwnerTable)
			.EnumDisplayName(Item->GetEnumDisplayName(InitOptions.NameTypeToDisplay))
			.HighlightText(SearchBox->GetText())
			.TextColor(FLinearColor(1.0f, 1.0f, 1.0f, (Item->PassedFilter() ? 1.0f : 0.5f)))
			.AssociatedNode(Item)
			.bIsInEnumViewer(InitOptions.Mode == EEnumViewerMode::EnumBrowsing)
			.bDynamicEnumLoading(bEnableEnumDynamicLoading)
			.OnDragDetected(this, &SEnumViewer::OnDragDetected);
	}

	TArray<TSharedPtr<FEnumViewerNode>> SEnumViewer::GetSelectedItems() const
	{
		return EnumList->GetSelectedItems();
	}

	int32 SEnumViewer::GetNumItems() const
	{
		return NumEnums;
	}

	FSlateColor SEnumViewer::GetViewButtonForegroundColor() const
	{
		static const FName InvertedForegroundName("InvertedForeground");
		static const FName DefaultForegroundName("DefaultForeground");

		return ViewOptionsComboButton->IsHovered() ? FEditorStyle::GetSlateColor(InvertedForegroundName) : FEditorStyle::GetSlateColor(DefaultForegroundName);
	}

	TSharedRef<SWidget> SEnumViewer::GetViewButtonContent()
	{
		// Get all menu extenders for this context menu from the content browser module

		FMenuBuilder MenuBuilder(/*bInShouldCloseWindowAfterMenuSelection=*/true, nullptr, nullptr, /*bCloseSelfOnly=*/ true);
		
		MenuBuilder.BeginSection("Filters", LOCTEXT("EnumViewerFiltersHeading", "Enum Filters"));
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

		MenuBuilder.BeginSection("DeveloperViewType", LOCTEXT("DeveloperViewTypeHeading", "Developer Folder Filter"));
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

	void SEnumViewer::SetCurrentDeveloperViewType(EEnumViewerDeveloperType NewType)
	{
		if (ensure(NewType < EEnumViewerDeveloperType::Max) && NewType != UEnumViewerSettings::Get().DeveloperFolderType)
		{
			UEnumViewerSettings::FEnumViewerModifier Modifier;
			Modifier.SetDeveloperFolderType(NewType);
		}
	}

	EEnumViewerDeveloperType SEnumViewer::GetCurrentDeveloperViewType() const
	{
		if (!InitOptions.bAllowViewOptions)
		{
			return EEnumViewerDeveloperType::All;
		}

		return GetDefault<UEnumViewerSettings>()->DeveloperFolderType;
	}

	bool SEnumViewer::IsCurrentDeveloperViewType(EEnumViewerDeveloperType ViewType) const
	{
		return GetCurrentDeveloperViewType() == ViewType;
	}

	void SEnumViewer::GetInternalOnlyEnums(TArray<TSoftObjectPtr<const UEnum>>& Enums)
	{
		if (!InitOptions.bAllowViewOptions)
		{
			return;
		}
		Enums = UEnumViewerSettings::Get().InternalOnlyEnums;
	}

	void SEnumViewer::GetInternalOnlyPaths(TArray<FDirectoryPath>& Paths)
	{
		if (!InitOptions.bAllowViewOptions)
		{
			return;
		}
		Paths = UEnumViewerSettings::Get().InternalOnlyPaths;
	}

	FText SEnumViewer::GetEnumCountText() const
	{
		const int32 NumSelectedEnums = GetSelectedItems().Num();
		if (NumSelectedEnums == 0)
		{
			return FText::Format(LOCTEXT("EnumCountLabel", "{0} {0}|plural(one=item,other=items)"), NumEnums);
		}
		else
		{
			return FText::Format(LOCTEXT("EnumCountLabelPlusSelection", "{0} {0}|plural(one=item,other=items) ({1} selected)"), NumEnums, NumSelectedEnums);
		}
	}

	void SEnumViewer::OnFilterTextChanged(const FText& InFilterText)
	{
		// Update the compiled filter and report any syntax error information back to the user
		TextFilterPtr->SetFilterText(InFilterText);
		SearchBox->SetError(TextFilterPtr->GetFilterErrorText());

		// Repopulate the list to show only what has not been filtered out.
		Refresh();
	}

	void SEnumViewer::OnFilterTextCommitted(const FText& InText, ETextCommit::Type CommitInfo)
	{
		if (CommitInfo == ETextCommit::OnEnter)
		{
			if (InitOptions.Mode == EEnumViewerMode::EnumPicker)
			{
				TArray<TSharedPtr<FEnumViewerNode>> SelectedList = EnumList->GetSelectedItems();
				if (SelectedList.Num() > 0)
				{
					TSharedPtr<FEnumViewerNode> FirstSelected = SelectedList[0];
					const UEnum* Enum = FirstSelected->GetEnum();

					// Try and ensure the enum is loaded
					if (bEnableEnumDynamicLoading && !Enum)
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
	
	void SEnumViewer::Populate()
	{
		// Empty the tree out so it can be redone.
		RootTreeItems.Empty();

		const bool ShowingInternalEnums = IsShowingInternalEnums();

		TArray<const UEnum*> InternalEnums;
		TArray<FDirectoryPath> InternalPaths;

		// If we aren't showing the internal enums, then we need to know what enums to consider Internal Only, so let's gather them up from the settings object.
		if (!ShowingInternalEnums)
		{
			TArray<TSoftObjectPtr<const UEnum>> InternalEnumNames;
			GetInternalOnlyPaths(InternalPaths);
			GetInternalOnlyEnums(InternalEnumNames);

			// Take the package names for the internal only enums and convert them into their UEnums
			for (const TSoftObjectPtr<const UEnum>& InternalEnumName : InternalEnumNames)
			{
				const TSharedPtr<FEnumViewerNodeData> EnumNode = FEnumHierarchy::Get().FindNodeByEnumPath(*InternalEnumName.ToString());
				if (EnumNode.IsValid())
				{
					if (const UEnum* Enum = EnumNode->GetEnum())
					{
						InternalEnums.Add(Enum);
					}
				}
			}
		}
		
		// Get the enum list, passing in certain filter options.
		RootTreeItems = FEnumHierarchy::Get().GetNodeList(InitOptions.PropertyHandle, true);
		NumEnums = RootTreeItems.Num();
		
		// Sort the list alphabetically.
		RootTreeItems.Sort(&FEnumViewerNode::SortPredicate);

		// Only display this option if the user wants it and in Picker Mode.
		if (InitOptions.bShowNoneOption && InitOptions.Mode == EEnumViewerMode::EnumPicker)
		{
			// @todo - It would seem smart to add this in before the other items, since it needs to be on top. However, that causes strange issues with saving/restoring expansion states. 
			// This is likely not very efficient since the list can have hundreds and even thousands of items.
			RootTreeItems.Insert(MakeShared<FEnumViewerNode>(), 0);
		}

		// Now that new items are in the list, we need to request a refresh.
		EnumList->RequestListRefresh();
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

			if (SelectedItems.Num() > 0 && SelectedItems[0].IsValid())
			{
				const TSharedRef<FEnumViewerNode> Item = SelectedItems[0].ToSharedRef();

				const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

				// Spawn a loaded user defined enum just like any other asset from the Content Browser.
				const FAssetData AssetData = AssetRegistryModule.Get().GetAssetByObjectPath(Item->GetEnumPath());
				if (AssetData.IsValid())
				{
					return FReply::Handled().BeginDragDrop(FContentBrowserDataDragDropOp::Legacy_New(MakeArrayView(&AssetData, 1)));
				}
			}
		}

		return FReply::Unhandled();
	}

	void SEnumViewer::DestroyEnumHierarchy()
	{
		FEnumHierarchy::DestroyInstance();
	}

	void SEnumViewer::Refresh()
	{
		bNeedsRefresh = true;
	}

	void SEnumViewer::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
	{
		// Will populate the enum hierarchy as needed.
		FEnumHierarchy::Get().UpdateEnumHierarchy();

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

	bool SEnumViewer::IsEnumAllowed(const UEnum* InEnum) const
	{
		return EnumViewer::Helpers::IsEnumAllowed(InitOptions, InEnum);
	}

	void SEnumViewer::HandleSettingChanged(FName PropertyName)
	{
		if ((PropertyName == "DisplayInternalClasses") ||
			(PropertyName == "DeveloperFolderType") ||
			(PropertyName == NAME_None))	// @todo: Needed if PostEditChange was called manually, for now
		{
			Refresh();
		}
	}

	void SEnumViewer::ToggleShowInternalEnums()
	{
		check(IsToggleShowInternalEnumsAllowed());
		GetMutableDefault<UEnumViewerSettings>()->bDisplayInternalEnums = !GetDefault<UEnumViewerSettings>()->bDisplayInternalEnums;
		GetMutableDefault<UEnumViewerSettings>()->PostEditChange();
	}

	bool SEnumViewer::IsToggleShowInternalEnumsAllowed() const
	{
		return bCanShowInternalEnums;
	}

	bool SEnumViewer::IsShowingInternalEnums() const
	{
		if (!InitOptions.bAllowViewOptions)
		{
			return true;
		}
		return IsToggleShowInternalEnumsAllowed() && GetDefault<UEnumViewerSettings>()->bDisplayInternalEnums;
	}
}

#undef LOCTEXT_NAMESPACE
