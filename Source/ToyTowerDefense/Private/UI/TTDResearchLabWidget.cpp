#include "UI/TTDResearchLabWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/GridPanel.h"
#include "Components/GridSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "TTDMenuPlayerController.h"
#include "TTDResearchSubsystem.h"
#include "UI/TTDActionButtonWidget.h"
#include "UI/TTDUIDisplayNames.h"
#include "UI/TTDUIStyle.h"

namespace
{
	UTTDActionButtonWidget* MakeResearchLabButton(UWidgetTree* WidgetTree, const FText& Label, const bool bEnabled, const FSimpleDelegate& Delegate, const ETTDActionButtonVariant Variant = ETTDActionButtonVariant::Secondary)
	{
		UTTDActionButtonWidget* Button = WidgetTree->ConstructWidget<UTTDActionButtonWidget>(UTTDActionButtonWidget::StaticClass());
		Button->Configure(Label, bEnabled, Delegate, Variant);
		return Button;
	}

	UBorder* MakeResearchLabPanel(UWidgetTree* WidgetTree, const FLinearColor& Color, const FMargin& Padding)
	{
		UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		TTDUIStyle::ApplyPanel(Panel, Color, Padding);
		return Panel;
	}

	UTextBlock* MakeResearchLabText(UWidgetTree* WidgetTree, const FText& TextValue, const int32 FontSize, const FLinearColor& Color = TTDUIStyle::Ink(), const ETextJustify::Type Justification = ETextJustify::Left)
	{
		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Text->SetText(TextValue);
		TTDUIStyle::ApplyText(Text, FontSize, Color, Justification, true);
		return Text;
	}

	FText PartIdsToText(const UGameInstance* GameInstance, const TArray<FName>& PartIds)
	{
		return FText::FromString(TTDUIDisplayNames::JoinPartIds(GameInstance, PartIds));
	}
}

void UTTDResearchLabWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	UBorder* Root = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	TTDUIStyle::ApplyPanel(Root, TTDUIStyle::Backdrop(), FMargin(18.0f));
	WidgetTree->RootWidget = Root;

	UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	Root->AddChild(RootOverlay);

	UHorizontalBox* RootRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	RootOverlay->AddChildToOverlay(RootRow);

	UBorder* SidePanel = MakeResearchLabPanel(WidgetTree, TTDUIStyle::Cardboard(), FMargin(16.0f, 18.0f));
	UHorizontalBoxSlot* SidePanelSlot = RootRow->AddChildToHorizontalBox(SidePanel);
	SidePanelSlot->SetPadding(FMargin(0.0f, 0.0f, 18.0f, 0.0f));
	SidePanelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

	UVerticalBox* SideNav = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	SidePanel->AddChild(SideNav);

	UTextBlock* NavTitle = MakeResearchLabText(WidgetTree, FText::FromString(TEXT("工坊")), 24, TTDUIStyle::Cream(), ETextJustify::Center);
	UVerticalBoxSlot* NavTitleSlot = SideNav->AddChildToVerticalBox(NavTitle);
	NavTitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 18.0f));
	NavTitleSlot->SetHorizontalAlignment(HAlign_Center);

	FSimpleDelegate BackDelegate;
	BackDelegate.BindUObject(this, &UTTDResearchLabWidget::HandleBackClicked);
	SideNav->AddChildToVerticalBox(MakeResearchLabButton(WidgetTree, FText::FromString(TEXT("返回")), true, BackDelegate, ETTDActionButtonVariant::Danger));

	FSimpleDelegate AlmanacDelegate;
	AlmanacDelegate.BindUObject(this, &UTTDResearchLabWidget::RenderAlmanac);
	UVerticalBoxSlot* AlmanacSlot = SideNav->AddChildToVerticalBox(MakeResearchLabButton(WidgetTree, FText::FromString(TEXT("图鉴")), true, AlmanacDelegate));
	AlmanacSlot->SetPadding(FMargin(0.0f, 18.0f, 0.0f, 0.0f));

	FSimpleDelegate ResearchDelegate;
	ResearchDelegate.BindUObject(this, &UTTDResearchLabWidget::RenderResearch);
	UVerticalBoxSlot* ResearchSlot = SideNav->AddChildToVerticalBox(MakeResearchLabButton(WidgetTree, FText::FromString(TEXT("研发")), true, ResearchDelegate, ETTDActionButtonVariant::Primary));
	ResearchSlot->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 0.0f));

	UBorder* ContentPanel = MakeResearchLabPanel(WidgetTree, TTDUIStyle::Paper(), FMargin(24.0f));
	UHorizontalBoxSlot* ContentPanelSlot = RootRow->AddChildToHorizontalBox(ContentPanel);
	ContentPanelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	ContentBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	ContentPanel->AddChild(ContentBox);

	DetailOverlay = MakeResearchLabPanel(WidgetTree, FLinearColor(0.188f, 0.149f, 0.114f, 0.66f), FMargin(32.0f));
	DetailOverlay->SetVisibility(ESlateVisibility::Collapsed);
	UOverlaySlot* DetailOverlaySlot = RootOverlay->AddChildToOverlay(DetailOverlay);
	DetailOverlaySlot->SetHorizontalAlignment(HAlign_Fill);
	DetailOverlaySlot->SetVerticalAlignment(VAlign_Fill);

	RenderResearch();
}

void UTTDResearchLabWidget::NativeConstruct()
{
	Super::NativeConstruct();
	TTDUIStyle::ResetFadeIn(this, EntranceElapsed);
	RegisterMessageListeners();
	UpdateResearchQueueRefreshTimer();
}

void UTTDResearchLabWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ResearchQueueRefreshTimerHandle);
	}
	UnregisterMessageListeners();
	Super::NativeDestruct();
}

void UTTDResearchLabWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	TTDUIStyle::TickFadeIn(this, EntranceElapsed, InDeltaTime);
}

UTTDResearchSubsystem* UTTDResearchLabWidget::GetResearchSubsystem() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<UTTDResearchSubsystem>() : nullptr;
}

void UTTDResearchLabWidget::RegisterMessageListeners()
{
	UnregisterMessageListeners();

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UGameplayMessageSubsystem* MessageSubsystem = GameInstance->GetSubsystem<UGameplayMessageSubsystem>())
		{
			QueueChangedListenerHandle = MessageSubsystem->RegisterListener<FTTDResearchQueueChangedMessage>(
				TAG_TTD_Message_Research_Queue_Changed,
				this,
				&UTTDResearchLabWidget::HandleQueueChangedMessage);

			CollectionChangedListenerHandle = MessageSubsystem->RegisterListener<FTTDResearchCollectionChangedMessage>(
				TAG_TTD_Message_Research_Collection_Changed,
				this,
				&UTTDResearchLabWidget::HandleCollectionChangedMessage);

			InventoryChangedListenerHandle = MessageSubsystem->RegisterListener<FTTDResearchInventoryChangedMessage>(
				TAG_TTD_Message_Research_Inventory_Changed,
				this,
				&UTTDResearchLabWidget::HandleInventoryChangedMessage);
		}
	}
}

void UTTDResearchLabWidget::UnregisterMessageListeners()
{
	QueueChangedListenerHandle.Unregister();
	CollectionChangedListenerHandle.Unregister();
	InventoryChangedListenerHandle.Unregister();
}

void UTTDResearchLabWidget::HandleBackClicked()
{
	if (ATTDMenuPlayerController* Controller = GetOwningPlayer<ATTDMenuPlayerController>())
	{
		Controller->ShowMainScreen();
	}
}

void UTTDResearchLabWidget::HandleQueueChangedMessage(FGameplayTag Channel, const FTTDResearchQueueChangedMessage& Message)
{
	if (bShowingResearch && !bShowingDiagramResearch && !bSuppressQueueChangedRefresh)
	{
		RenderResearch();
	}
}

void UTTDResearchLabWidget::HandleCollectionChangedMessage(FGameplayTag Channel, const FTTDResearchCollectionChangedMessage& Message)
{
	if (bShowingDiagramResearch)
	{
		RenderDiagramResearch();
	}
	else if (!bShowingResearch)
	{
		RenderAlmanac();
	}
}

void UTTDResearchLabWidget::HandleInventoryChangedMessage(FGameplayTag Channel, const FTTDResearchInventoryChangedMessage& Message)
{
	if (bShowingDiagramResearch)
	{
		RenderDiagramResearch();
	}
}

void UTTDResearchLabWidget::RenderAlmanac()
{
	bShowingResearch = false;
	bShowingDiagramResearch = false;
	LastFeedback = FText::GetEmpty();
	UpdateResearchQueueRefreshTimer();

	if (!ContentBox)
	{
		return;
	}

	ContentBox->ClearChildren();
	QueueSlotButtons.Reset();

	UTextBlock* Title = MakeResearchLabText(WidgetTree, FText::FromString(TEXT("研究所 / 图鉴")), 32);
	ContentBox->AddChildToVerticalBox(Title);

	UHorizontalBox* Tabs = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UVerticalBoxSlot* TabsSlot = ContentBox->AddChildToVerticalBox(Tabs);
	TabsSlot->SetPadding(FMargin(0.0f, 14.0f, 0.0f, 18.0f));

	FSimpleDelegate DiagramDelegate;
	DiagramDelegate.BindUObject(this, &UTTDResearchLabWidget::SetCollectionCategory, ETTDCollectionCategory::Diagram);
	Tabs->AddChildToHorizontalBox(MakeResearchLabButton(WidgetTree, FText::FromString(TEXT("图纸")), true, DiagramDelegate, ActiveCollectionCategory == ETTDCollectionCategory::Diagram ? ETTDActionButtonVariant::Primary : ETTDActionButtonVariant::Secondary));

	FSimpleDelegate ToyBoxDelegate;
	ToyBoxDelegate.BindUObject(this, &UTTDResearchLabWidget::SetCollectionCategory, ETTDCollectionCategory::ToyBox);
	UHorizontalBoxSlot* ToyBoxTabSlot = Tabs->AddChildToHorizontalBox(MakeResearchLabButton(WidgetTree, FText::FromString(TEXT("玩具盒")), true, ToyBoxDelegate, ActiveCollectionCategory == ETTDCollectionCategory::ToyBox ? ETTDActionButtonVariant::Primary : ETTDActionButtonVariant::Secondary));
	ToyBoxTabSlot->SetPadding(FMargin(12.0f, 0.0f, 0.0f, 0.0f));

	UBorder* GridPanel = MakeResearchLabPanel(WidgetTree, TTDUIStyle::Cream(), FMargin(14.0f));
	UVerticalBoxSlot* GridPanelSlot = ContentBox->AddChildToVerticalBox(GridPanel);
	GridPanelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	UScrollBox* ScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
	GridPanel->AddChild(ScrollBox);

	UGridPanel* Grid = WidgetTree->ConstructWidget<UGridPanel>(UGridPanel::StaticClass());
	ScrollBox->AddChild(Grid);

	if (UTTDResearchSubsystem* ResearchSubsystem = GetResearchSubsystem())
	{
		const TArray<FTTDCollectionEntry> Entries = ResearchSubsystem->GetCollectionEntries(ActiveCollectionCategory);
		for (int32 Index = 0; Index < Entries.Num(); ++Index)
		{
			const FTTDCollectionEntry Entry = Entries[Index];
			const FText ButtonLabel = Entry.bUnlocked ? Entry.DisplayName : FText::FromString(TEXT("未解锁"));
			FSimpleDelegate EntryDelegate;
			EntryDelegate.BindWeakLambda(this, [this, Entry]()
			{
				ShowEntryDetails(Entry);
			});

			UTTDActionButtonWidget* EntryButton = MakeResearchLabButton(WidgetTree, ButtonLabel, Entry.bUnlocked, EntryDelegate, ETTDActionButtonVariant::QueueSlot);
			UGridSlot* GridSlot = Grid->AddChildToGrid(EntryButton, Index / 5, Index % 5);
			GridSlot->SetPadding(FMargin(8.0f));
		}
	}

	UBorder* FeedbackPanel = MakeResearchLabPanel(WidgetTree, TTDUIStyle::Paper(), FMargin(18.0f, 12.0f));
	UVerticalBoxSlot* FeedbackPanelSlot = ContentBox->AddChildToVerticalBox(FeedbackPanel);
	FeedbackPanelSlot->SetPadding(FMargin(0.0f, 16.0f, 0.0f, 0.0f));

	FeedbackText = MakeResearchLabText(WidgetTree, LastFeedback.IsEmpty() ? FText::FromString(TEXT("点击已解锁内容查看说明。")) : LastFeedback, 17, TTDUIStyle::MutedInk());
	FeedbackPanel->AddChild(FeedbackText);
}

void UTTDResearchLabWidget::ShowEntryDetails(const FTTDCollectionEntry Entry)
{
	if (!DetailOverlay || !Entry.bUnlocked)
	{
		return;
	}

	DetailOverlay->ClearChildren();
	DetailOverlay->SetVisibility(ESlateVisibility::Visible);

	UBorder* CardPanel = MakeResearchLabPanel(WidgetTree, TTDUIStyle::Paper(), FMargin(26.0f, 22.0f));
	UVerticalBox* CardBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	CardPanel->AddChild(CardBox);

	const FText TypeText = Entry.Category == ETTDCollectionCategory::Diagram
		? FText::FromString(TEXT("图纸详情"))
		: FText::FromString(TEXT("玩具盒详情"));
	CardBox->AddChildToVerticalBox(MakeResearchLabText(WidgetTree, TypeText, 17, TTDUIStyle::MutedInk(), ETextJustify::Center));

	UVerticalBoxSlot* NameSlot = CardBox->AddChildToVerticalBox(MakeResearchLabText(WidgetTree, Entry.DisplayName, 30, TTDUIStyle::Ink(), ETextJustify::Center));
	NameSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 12.0f));

	UVerticalBoxSlot* DescriptionSlot = CardBox->AddChildToVerticalBox(MakeResearchLabText(WidgetTree, Entry.Description, 17, TTDUIStyle::MutedInk(), ETextJustify::Center));
	DescriptionSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));

	AddPartScroller(CardBox, Entry);

	FSimpleDelegate CloseDelegate;
	CloseDelegate.BindUObject(this, &UTTDResearchLabWidget::HideEntryDetails);
	UVerticalBoxSlot* CloseSlot = CardBox->AddChildToVerticalBox(MakeResearchLabButton(WidgetTree, FText::FromString(TEXT("关闭")), true, CloseDelegate, ETTDActionButtonVariant::Ghost));
	CloseSlot->SetHorizontalAlignment(HAlign_Center);
	CloseSlot->SetPadding(FMargin(0.0f, 18.0f, 0.0f, 0.0f));

	DetailOverlay->AddChild(CardPanel);
}

void UTTDResearchLabWidget::HideEntryDetails()
{
	if (DetailOverlay)
	{
		DetailOverlay->ClearChildren();
		DetailOverlay->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UTTDResearchLabWidget::AddPartScroller(UVerticalBox* ParentBox, const FTTDCollectionEntry& Entry) const
{
	if (!ParentBox)
	{
		return;
	}

	const FText PartLabel = Entry.Category == ETTDCollectionCategory::Diagram
		? FText::FromString(TEXT("组装零件"))
		: FText::FromString(TEXT("可开出零件"));
	UVerticalBoxSlot* LabelSlot = ParentBox->AddChildToVerticalBox(MakeResearchLabText(WidgetTree, PartLabel, 17, TTDUIStyle::Ink()));
	LabelSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

	UScrollBox* PartScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
	PartScroll->SetOrientation(Orient_Horizontal);
	UHorizontalBox* PartRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	PartScroll->AddChild(PartRow);

	if (Entry.PartIds.IsEmpty())
	{
		PartRow->AddChildToHorizontalBox(MakeResearchLabText(WidgetTree, FText::FromString(TEXT("暂无")), 16, TTDUIStyle::MutedInk()));
	}
	else
	{
		for (const FName PartId : Entry.PartIds)
		{
			UBorder* PartChip = MakeResearchLabPanel(WidgetTree, TTDUIStyle::LightPaper(), FMargin(14.0f, 10.0f));
			PartChip->AddChild(MakeResearchLabText(WidgetTree, TTDUIDisplayNames::PartName(GetGameInstance(), PartId), 16, TTDUIStyle::Ink(), ETextJustify::Center));
			UHorizontalBoxSlot* ChipSlot = PartRow->AddChildToHorizontalBox(PartChip);
			ChipSlot->SetPadding(FMargin(0.0f, 0.0f, 10.0f, 0.0f));
		}
	}

	ParentBox->AddChildToVerticalBox(PartScroll);
}

void UTTDResearchLabWidget::RenderResearch()
{
	bShowingResearch = true;
	bShowingDiagramResearch = false;

	if (!ContentBox)
	{
		UpdateResearchQueueRefreshTimer();
		return;
	}

	ContentBox->ClearChildren();
	QueueSlotButtons.Reset();

	UTextBlock* Title = MakeResearchLabText(WidgetTree, FText::FromString(TEXT("研究所 / 研发")), 32);
	ContentBox->AddChildToVerticalBox(Title);

	UHorizontalBox* ModeTabs = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UVerticalBoxSlot* ModeTabsSlot = ContentBox->AddChildToVerticalBox(ModeTabs);
	ModeTabsSlot->SetPadding(FMargin(0.0f, 14.0f, 0.0f, 18.0f));

	FSimpleDelegate ToyBoxModeDelegate;
	ToyBoxModeDelegate.BindUObject(this, &UTTDResearchLabWidget::RenderResearch);
	ModeTabs->AddChildToHorizontalBox(MakeResearchLabButton(WidgetTree, FText::FromString(TEXT("玩具盒")), true, ToyBoxModeDelegate, ETTDActionButtonVariant::Primary));

	FSimpleDelegate DiagramModeDelegate;
	DiagramModeDelegate.BindUObject(this, &UTTDResearchLabWidget::RenderDiagramResearch);
	UHorizontalBoxSlot* DiagramSlot = ModeTabs->AddChildToHorizontalBox(MakeResearchLabButton(WidgetTree, FText::FromString(TEXT("图纸")), true, DiagramModeDelegate));
	DiagramSlot->SetPadding(FMargin(12.0f, 0.0f, 0.0f, 0.0f));

	UTTDResearchSubsystem* ResearchSubsystem = GetResearchSubsystem();
	if (!ResearchSubsystem)
	{
		LastFeedback = FText::FromString(TEXT("研究所系统未初始化。"));
		ContentBox->AddChildToVerticalBox(MakeResearchLabText(WidgetTree, LastFeedback, 18, TTDUIStyle::Danger()));
		UpdateResearchQueueRefreshTimer();
		return;
	}

	UBorder* QueuePanel = MakeResearchLabPanel(WidgetTree, TTDUIStyle::Cream(), FMargin(18.0f));
	UVerticalBoxSlot* QueuePanelSlot = ContentBox->AddChildToVerticalBox(QueuePanel);
	QueuePanelSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 18.0f));

	UVerticalBox* QueueBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	QueuePanel->AddChild(QueueBox);

	UTextBlock* QueueLabel = MakeResearchLabText(WidgetTree, FText::FromString(TEXT("制作队列")), 22, TTDUIStyle::Ink());
	QueueBox->AddChildToVerticalBox(QueueLabel);

	UHorizontalBox* QueueRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UVerticalBoxSlot* QueueRowSlot = QueueBox->AddChildToVerticalBox(QueueRow);
	QueueRowSlot->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 0.0f));

	const TArray<FTTDCraftQueueItem> QueueItems = ResearchSubsystem->GetCraftingQueue();
	for (int32 SlotIndex = 0; SlotIndex < ResearchSubsystem->GetCraftingQueueMaxSlots(); ++SlotIndex)
	{
		FText SlotText = FText::FromString(TEXT("+"));
		bool bSlotEnabled = false;
		bool bCompleted = false;
		FSimpleDelegate SlotDelegate;

		if (QueueItems.IsValidIndex(SlotIndex))
		{
			const FTTDCraftQueueItem QueueItem = QueueItems[SlotIndex];
			SlotText = BuildQueueSlotText(QueueItem);
			bSlotEnabled = QueueItem.bCompleted;
			bCompleted = QueueItem.bCompleted;
			SlotDelegate.BindWeakLambda(this, [this, QueueItem]()
			{
				ClaimQueueItem(QueueItem.QueueId);
			});
		}

		UTTDActionButtonWidget* QueueButton = MakeResearchLabButton(WidgetTree, SlotText, bSlotEnabled, SlotDelegate, ETTDActionButtonVariant::QueueSlot);
		QueueButton->SetPulseActive(bCompleted);
		QueueSlotButtons.Add(QueueButton);

		UHorizontalBoxSlot* QueueButtonSlot = QueueRow->AddChildToHorizontalBox(QueueButton);
		QueueButtonSlot->SetPadding(FMargin(0.0f, 0.0f, 12.0f, 0.0f));
	}

	UHorizontalBox* BodyRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UVerticalBoxSlot* BodySlot = ContentBox->AddChildToVerticalBox(BodyRow);
	BodySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	UBorder* PreviewPanel = MakeResearchLabPanel(WidgetTree, TTDUIStyle::Cream(), FMargin(20.0f));
	UHorizontalBoxSlot* PreviewSlot = BodyRow->AddChildToHorizontalBox(PreviewPanel);
	PreviewSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	PreviewSlot->SetPadding(FMargin(0.0f, 0.0f, 16.0f, 0.0f));

	UVerticalBox* PreviewBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	PreviewPanel->AddChild(PreviewBox);

	const TArray<FTTDToyBoxDefinition> ToyBoxes = ResearchSubsystem->GetToyBoxes();
	if (!ToyBoxes.ContainsByPredicate([this](const FTTDToyBoxDefinition& ToyBox) { return ToyBox.ToyBoxId == SelectedToyBoxId; }) && ToyBoxes.Num() > 0)
	{
		SelectedToyBoxId = ToyBoxes[0].ToyBoxId;
	}

	const FTTDToyBoxDefinition* SelectedToyBox = ToyBoxes.FindByPredicate([this](const FTTDToyBoxDefinition& ToyBox)
	{
		return ToyBox.ToyBoxId == SelectedToyBoxId;
	});

	if (SelectedToyBox)
	{
		PreviewBox->AddChildToVerticalBox(MakeResearchLabText(WidgetTree, SelectedToyBox->DisplayName, 28));

		UTextBlock* Description = MakeResearchLabText(WidgetTree, SelectedToyBox->Description, 17, TTDUIStyle::MutedInk());
		UVerticalBoxSlot* DescriptionSlot = PreviewBox->AddChildToVerticalBox(Description);
		DescriptionSlot->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 16.0f));

		PreviewBox->AddChildToVerticalBox(MakeResearchLabText(
			WidgetTree,
			FText::Format(FText::FromString(TEXT("可开出零件：{0}")), PartIdsToText(GetGameInstance(), SelectedToyBox->PossiblePartIds)),
			17,
			TTDUIStyle::Ink()));

		UTextBlock* DurationText = MakeResearchLabText(
			WidgetTree,
			FText::Format(FText::FromString(TEXT("制作所需时间：{0}")), FormatSeconds(SelectedToyBox->CraftDurationSeconds)),
			17,
			TTDUIStyle::Ink());
		UVerticalBoxSlot* DurationSlot = PreviewBox->AddChildToVerticalBox(DurationText);
		DurationSlot->SetPadding(FMargin(0.0f, 10.0f, 0.0f, 18.0f));

		FSimpleDelegate EnqueueDelegate;
		EnqueueDelegate.BindUObject(this, &UTTDResearchLabWidget::EnqueueSelectedToyBox);
		PreviewBox->AddChildToVerticalBox(MakeResearchLabButton(WidgetTree, FText::FromString(TEXT("加入制作队列")), true, EnqueueDelegate, ETTDActionButtonVariant::Primary));
	}

	UBorder* FeedbackPanel = MakeResearchLabPanel(WidgetTree, TTDUIStyle::Paper(), FMargin(16.0f, 12.0f));
	UVerticalBoxSlot* FeedbackPanelSlot = PreviewBox->AddChildToVerticalBox(FeedbackPanel);
	FeedbackPanelSlot->SetPadding(FMargin(0.0f, 18.0f, 0.0f, 0.0f));

	FeedbackText = MakeResearchLabText(WidgetTree, LastFeedback.IsEmpty() ? FText::FromString(TEXT("选择右侧玩具盒后可加入制作队列。")) : LastFeedback, 17, TTDUIStyle::MutedInk());
	FeedbackPanel->AddChild(FeedbackText);

	UBorder* ListPanel = MakeResearchLabPanel(WidgetTree, TTDUIStyle::Cream(), FMargin(14.0f));
	UHorizontalBoxSlot* ListSlot = BodyRow->AddChildToHorizontalBox(ListPanel);
	ListSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

	UScrollBox* ToyBoxScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
	ListPanel->AddChild(ToyBoxScroll);

	UVerticalBox* ToyBoxList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	ToyBoxScroll->AddChild(ToyBoxList);

	for (const FTTDCollectionEntry& Entry : ResearchSubsystem->GetCollectionEntries(ETTDCollectionCategory::ToyBox))
	{
		FSimpleDelegate SelectDelegate;
		SelectDelegate.BindUObject(this, &UTTDResearchLabWidget::SetSelectedToyBox, Entry.ItemId);

		const FText Label = Entry.bUnlocked
			? Entry.DisplayName
			: FText::Format(FText::FromString(TEXT("{0}\n未解锁")), Entry.DisplayName);

		UVerticalBoxSlot* ItemSlot = ToyBoxList->AddChildToVerticalBox(MakeResearchLabButton(WidgetTree, Label, Entry.bUnlocked, SelectDelegate));
		ItemSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
	}

	UpdateResearchQueueRefreshTimer();
}

void UTTDResearchLabWidget::RefreshResearchQueueButtons()
{
	UTTDResearchSubsystem* ResearchSubsystem = GetResearchSubsystem();
	if (!ResearchSubsystem)
	{
		return;
	}

	const TArray<FTTDCraftQueueItem> QueueItems = ResearchSubsystem->GetCraftingQueue();
	for (int32 SlotIndex = 0; SlotIndex < QueueSlotButtons.Num(); ++SlotIndex)
	{
		UTTDActionButtonWidget* QueueButton = QueueSlotButtons[SlotIndex];
		if (!QueueButton)
		{
			continue;
		}

		FText SlotText = FText::FromString(TEXT("+"));
		bool bSlotEnabled = false;
		bool bCompleted = false;

		if (QueueItems.IsValidIndex(SlotIndex))
		{
			const FTTDCraftQueueItem QueueItem = QueueItems[SlotIndex];
			SlotText = BuildQueueSlotText(QueueItem);
			bSlotEnabled = QueueItem.bCompleted;
			bCompleted = QueueItem.bCompleted;
		}

		QueueButton->SetLabelAndEnabled(SlotText, bSlotEnabled);
		QueueButton->SetPulseActive(bCompleted);
	}
}

void UTTDResearchLabWidget::UpdateResearchQueueRefreshTimer()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FTimerManager& TimerManager = World->GetTimerManager();
	bool bHasActiveQueueItem = false;
	if (const UTTDResearchSubsystem* ResearchSubsystem = GetResearchSubsystem())
	{
		for (const FTTDCraftQueueItem& QueueItem : ResearchSubsystem->GetCraftingQueue())
		{
			if (!QueueItem.bCompleted)
			{
				bHasActiveQueueItem = true;
				break;
			}
		}
	}

	const bool bShouldRefreshQueue = bShowingResearch
		&& !bShowingDiagramResearch
		&& QueueSlotButtons.Num() > 0
		&& bHasActiveQueueItem;
	if (bShouldRefreshQueue)
	{
		if (!TimerManager.IsTimerActive(ResearchQueueRefreshTimerHandle))
		{
			TimerManager.SetTimer(
				ResearchQueueRefreshTimerHandle,
				this,
				&UTTDResearchLabWidget::RefreshResearchQueueButtons,
				1.0f,
				true);
		}
	}
	else
	{
		TimerManager.ClearTimer(ResearchQueueRefreshTimerHandle);
	}
}

FText UTTDResearchLabWidget::BuildQueueSlotText(const FTTDCraftQueueItem& QueueItem) const
{
	return FText::FromString(FString::Printf(
		TEXT("%s\n%s%s"),
		*QueueItem.ToyBoxName.ToString(),
		*FormatSeconds(QueueItem.RemainingSeconds).ToString(),
		QueueItem.bCompleted ? TEXT("\n完成") : TEXT("")));
}

void UTTDResearchLabWidget::RenderDiagramResearch()
{
	bShowingResearch = true;
	bShowingDiagramResearch = true;
	UpdateResearchQueueRefreshTimer();

	if (!ContentBox)
	{
		return;
	}

	ContentBox->ClearChildren();
	QueueSlotButtons.Reset();

	ContentBox->AddChildToVerticalBox(MakeResearchLabText(WidgetTree, FText::FromString(TEXT("研究所 / 图纸研发")), 32));

	UHorizontalBox* ModeTabs = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UVerticalBoxSlot* ModeTabsSlot = ContentBox->AddChildToVerticalBox(ModeTabs);
	ModeTabsSlot->SetPadding(FMargin(0.0f, 14.0f, 0.0f, 18.0f));

	FSimpleDelegate ToyBoxModeDelegate;
	ToyBoxModeDelegate.BindUObject(this, &UTTDResearchLabWidget::RenderResearch);
	ModeTabs->AddChildToHorizontalBox(MakeResearchLabButton(WidgetTree, FText::FromString(TEXT("玩具盒")), true, ToyBoxModeDelegate));

	FSimpleDelegate DiagramModeDelegate;
	DiagramModeDelegate.BindUObject(this, &UTTDResearchLabWidget::RenderDiagramResearch);
	UHorizontalBoxSlot* DiagramSlot = ModeTabs->AddChildToHorizontalBox(MakeResearchLabButton(WidgetTree, FText::FromString(TEXT("图纸")), true, DiagramModeDelegate, ETTDActionButtonVariant::Primary));
	DiagramSlot->SetPadding(FMargin(12.0f, 0.0f, 0.0f, 0.0f));

	UTTDResearchSubsystem* ResearchSubsystem = GetResearchSubsystem();
	if (!ResearchSubsystem)
	{
		LastFeedback = FText::FromString(TEXT("研究所系统未初始化。"));
		ContentBox->AddChildToVerticalBox(MakeResearchLabText(WidgetTree, LastFeedback, 18, TTDUIStyle::Danger()));
		return;
	}

	const TArray<FTTDDiagramDefinition> Diagrams = ResearchSubsystem->GetDiagrams();
	if (!Diagrams.ContainsByPredicate([this](const FTTDDiagramDefinition& Diagram) { return Diagram.DiagramId == SelectedDiagramId; }) && Diagrams.Num() > 0)
	{
		SelectedDiagramId = Diagrams[0].DiagramId;
	}

	UHorizontalBox* BodyRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UVerticalBoxSlot* BodySlot = ContentBox->AddChildToVerticalBox(BodyRow);
	BodySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	UBorder* PreviewPanel = MakeResearchLabPanel(WidgetTree, TTDUIStyle::Cream(), FMargin(20.0f));
	UHorizontalBoxSlot* PreviewSlot = BodyRow->AddChildToHorizontalBox(PreviewPanel);
	PreviewSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	PreviewSlot->SetPadding(FMargin(0.0f, 0.0f, 16.0f, 0.0f));

	UVerticalBox* PreviewBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	PreviewPanel->AddChild(PreviewBox);

	const FTTDDiagramDefinition* SelectedDiagram = Diagrams.FindByPredicate([this](const FTTDDiagramDefinition& Diagram)
	{
		return Diagram.DiagramId == SelectedDiagramId;
	});

	if (SelectedDiagram)
	{
		PreviewBox->AddChildToVerticalBox(MakeResearchLabText(WidgetTree, SelectedDiagram->DisplayName, 28));

		UTextBlock* Description = MakeResearchLabText(WidgetTree, SelectedDiagram->Description, 17, TTDUIStyle::MutedInk());
		UVerticalBoxSlot* DescriptionSlot = PreviewBox->AddChildToVerticalBox(Description);
		DescriptionSlot->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 16.0f));

		PreviewBox->AddChildToVerticalBox(MakeResearchLabText(WidgetTree, BuildDiagramCostText(*SelectedDiagram, *ResearchSubsystem), 17, TTDUIStyle::Ink()));

		FText FailureReason;
		const bool bCanResearch = ResearchSubsystem->CanResearchDiagram(SelectedDiagram->DiagramId, FailureReason);
		const FText ButtonLabel = bCanResearch
			? FText::FromString(TEXT("研发图纸"))
			: (FailureReason.IsEmpty() ? FText::FromString(TEXT("不可研发")) : FailureReason);

		FSimpleDelegate ResearchDelegate;
		ResearchDelegate.BindUObject(this, &UTTDResearchLabWidget::ResearchSelectedDiagram);
		UVerticalBoxSlot* ButtonSlot = PreviewBox->AddChildToVerticalBox(MakeResearchLabButton(WidgetTree, ButtonLabel, bCanResearch, ResearchDelegate, ETTDActionButtonVariant::Primary));
		ButtonSlot->SetPadding(FMargin(0.0f, 18.0f, 0.0f, 0.0f));
	}

	UBorder* FeedbackPanel = MakeResearchLabPanel(WidgetTree, TTDUIStyle::Paper(), FMargin(16.0f, 12.0f));
	UVerticalBoxSlot* FeedbackPanelSlot = PreviewBox->AddChildToVerticalBox(FeedbackPanel);
	FeedbackPanelSlot->SetPadding(FMargin(0.0f, 18.0f, 0.0f, 0.0f));
	FeedbackText = MakeResearchLabText(WidgetTree, LastFeedback.IsEmpty() ? FText::FromString(TEXT("选择右侧图纸查看研发消耗。")) : LastFeedback, 17, TTDUIStyle::MutedInk());
	FeedbackPanel->AddChild(FeedbackText);

	UBorder* ListPanel = MakeResearchLabPanel(WidgetTree, TTDUIStyle::Cream(), FMargin(14.0f));
	UHorizontalBoxSlot* ListSlot = BodyRow->AddChildToHorizontalBox(ListPanel);
	ListSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

	UScrollBox* DiagramScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
	ListPanel->AddChild(DiagramScroll);

	UVerticalBox* DiagramList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	DiagramScroll->AddChild(DiagramList);

	for (const FTTDCollectionEntry& Entry : ResearchSubsystem->GetCollectionEntries(ETTDCollectionCategory::Diagram))
	{
		FSimpleDelegate SelectDelegate;
		SelectDelegate.BindUObject(this, &UTTDResearchLabWidget::SetSelectedDiagram, Entry.ItemId);

		const FText Label = Entry.bUnlocked
			? FText::Format(FText::FromString(TEXT("{0}\n已解锁")), Entry.DisplayName)
			: Entry.DisplayName;

		UVerticalBoxSlot* ItemSlot = DiagramList->AddChildToVerticalBox(MakeResearchLabButton(WidgetTree, Label, true, SelectDelegate, Entry.ItemId == SelectedDiagramId ? ETTDActionButtonVariant::Primary : ETTDActionButtonVariant::Secondary));
		ItemSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
	}
}

void UTTDResearchLabWidget::SetCollectionCategory(const ETTDCollectionCategory Category)
{
	ActiveCollectionCategory = Category;
	LastFeedback = FText::GetEmpty();
	RenderAlmanac();
}

void UTTDResearchLabWidget::SetSelectedToyBox(const FName ToyBoxId)
{
	SelectedToyBoxId = ToyBoxId;
	LastFeedback = FText::GetEmpty();
	RenderResearch();
}

void UTTDResearchLabWidget::SetSelectedDiagram(const FName DiagramId)
{
	SelectedDiagramId = DiagramId;
	LastFeedback = FText::GetEmpty();
	RenderDiagramResearch();
}

void UTTDResearchLabWidget::EnqueueSelectedToyBox()
{
	if (UTTDResearchSubsystem* ResearchSubsystem = GetResearchSubsystem())
	{
		FText FailureReason;
		bSuppressQueueChangedRefresh = true;
		const bool bEnqueued = ResearchSubsystem->EnqueueToyBox(SelectedToyBoxId, FailureReason);
		bSuppressQueueChangedRefresh = false;

		if (bEnqueued)
		{
			LastFeedback = FText::FromString(TEXT("已加入制作队列。"));
		}
		else
		{
			LastFeedback = FailureReason;
		}
	}

	RenderResearch();
}

void UTTDResearchLabWidget::ClaimQueueItem(const FGuid QueueId)
{
	if (UTTDResearchSubsystem* ResearchSubsystem = GetResearchSubsystem())
	{
		TArray<FTTDPartDefinition> NewParts;
		bSuppressQueueChangedRefresh = true;
		const bool bClaimed = ResearchSubsystem->ClaimCompletedToyBox(QueueId, NewParts);
		bSuppressQueueChangedRefresh = false;

		if (bClaimed)
		{
			LastFeedback = FText::FromString(TEXT("制作完成，玩具盒库存已增加。"));
		}
	}

	RenderResearch();
}

void UTTDResearchLabWidget::ResearchSelectedDiagram()
{
	if (UTTDResearchSubsystem* ResearchSubsystem = GetResearchSubsystem())
	{
		FText FailureReason;
		const bool bResearched = ResearchSubsystem->ResearchDiagram(SelectedDiagramId, FailureReason);
		LastFeedback = bResearched
			? FText::FromString(TEXT("图纸研发完成。"))
			: FailureReason;
	}

	RenderDiagramResearch();
}

FText UTTDResearchLabWidget::DescribeEntry(const FTTDCollectionEntry& Entry) const
{
	const FText TypeText = Entry.Category == ETTDCollectionCategory::Diagram
		? FText::FromString(TEXT("图纸"))
		: FText::FromString(TEXT("玩具盒"));

	const FText PartLabel = Entry.Category == ETTDCollectionCategory::Diagram
		? FText::FromString(TEXT("所需零件"))
		: FText::FromString(TEXT("可开出零件"));

	return FText::Format(
		FText::FromString(TEXT("{0}：{1}\n{2}\n{3}：{4}")),
		TypeText,
		Entry.DisplayName,
		Entry.Description,
		PartLabel,
		PartIdsToText(GetGameInstance(), Entry.PartIds));
}

FText UTTDResearchLabWidget::FormatSeconds(const float Seconds) const
{
	const int32 TotalSeconds = FMath::Max(0, FMath::CeilToInt(Seconds));
	const int32 Hours = TotalSeconds / 3600;
	const int32 Minutes = (TotalSeconds % 3600) / 60;
	const int32 RemainingSeconds = TotalSeconds % 60;
	return FText::FromString(FString::Printf(TEXT("%02d:%02d:%02d"), Hours, Minutes, RemainingSeconds));
}

FText UTTDResearchLabWidget::BuildDiagramCostText(const FTTDDiagramDefinition& Diagram, const UTTDResearchSubsystem& ResearchSubsystem) const
{
	TArray<FTTDNameStack> Costs = Diagram.PartCosts;
	if (Costs.IsEmpty())
	{
		for (const FName PartId : Diagram.RequiredPartIds)
		{
			if (!PartId.IsNone())
			{
				Costs.Add(FTTDNameStack(PartId, 1));
			}
		}
	}

	Costs.Sort(
		[](const FTTDNameStack& Left, const FTTDNameStack& Right)
		{
			return Left.Id.LexicalLess(Right.Id);
		});

	TArray<FString> Lines;
	for (const FTTDNameStack& Cost : Costs)
	{
		if (!Cost.Id.IsNone() && Cost.Count > 0)
		{
			Lines.Add(FString::Printf(
				TEXT("%s：%d/%d"),
				*TTDUIDisplayNames::PartName(GetGameInstance(), Cost.Id).ToString(),
				ResearchSubsystem.GetPartCount(Cost.Id),
				Cost.Count));
		}
	}

	return Lines.IsEmpty()
		? FText::FromString(TEXT("研发消耗：无"))
		: FText::FromString(FString::Printf(TEXT("研发消耗：\n%s"), *FString::Join(Lines, TEXT("\n"))));
}
