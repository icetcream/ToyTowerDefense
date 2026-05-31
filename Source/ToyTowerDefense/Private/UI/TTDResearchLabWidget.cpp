#include "UI/TTDResearchLabWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/GridPanel.h"
#include "Components/GridSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "TTDMenuPlayerController.h"
#include "TTDResearchSubsystem.h"
#include "UI/TTDActionButtonWidget.h"

namespace
{
	UTTDActionButtonWidget* MakeButton(UWidgetTree* WidgetTree, const FText& Label, const bool bEnabled, const FSimpleDelegate& Delegate)
	{
		UTTDActionButtonWidget* Button = WidgetTree->ConstructWidget<UTTDActionButtonWidget>(UTTDActionButtonWidget::StaticClass());
		Button->Configure(Label, bEnabled, Delegate);
		return Button;
	}

	FText PartIdsToText(const TArray<FName>& PartIds)
	{
		TArray<FString> Names;
		for (const FName PartId : PartIds)
		{
			Names.Add(PartId.ToString());
		}
		return FText::FromString(FString::Join(Names, TEXT("、")));
	}

	void UseDarkText(UTextBlock* Text)
	{
		if (Text)
		{
			Text->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
		}
	}
}

void UTTDResearchLabWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	UBorder* Root = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Root->SetBrushColor(FLinearColor(0.86f, 0.91f, 1.0f, 1.0f));
	Root->SetPadding(FMargin(20.0f));
	WidgetTree->RootWidget = Root;

	UHorizontalBox* RootRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Root->AddChild(RootRow);

	UVerticalBox* SideNav = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UHorizontalBoxSlot* SideSlot = RootRow->AddChildToHorizontalBox(SideNav);
	SideSlot->SetPadding(FMargin(0.0f, 0.0f, 16.0f, 0.0f));
	SideSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

	FSimpleDelegate BackDelegate;
	BackDelegate.BindUObject(this, &UTTDResearchLabWidget::HandleBackClicked);
	SideNav->AddChildToVerticalBox(MakeButton(WidgetTree, FText::FromString(TEXT("返回")), true, BackDelegate));

	FSimpleDelegate AlmanacDelegate;
	AlmanacDelegate.BindUObject(this, &UTTDResearchLabWidget::RenderAlmanac);
	UVerticalBoxSlot* AlmanacSlot = SideNav->AddChildToVerticalBox(MakeButton(WidgetTree, FText::FromString(TEXT("图鉴")), true, AlmanacDelegate));
	AlmanacSlot->SetPadding(FMargin(0.0f, 20.0f, 0.0f, 0.0f));

	FSimpleDelegate ResearchDelegate;
	ResearchDelegate.BindUObject(this, &UTTDResearchLabWidget::RenderResearch);
	UVerticalBoxSlot* ResearchSlot = SideNav->AddChildToVerticalBox(MakeButton(WidgetTree, FText::FromString(TEXT("研发")), true, ResearchDelegate));
	ResearchSlot->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 0.0f));

	ContentBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UHorizontalBoxSlot* ContentSlot = RootRow->AddChildToHorizontalBox(ContentBox);
	ContentSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

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
		}
	}

	RenderResearch();
}

void UTTDResearchLabWidget::NativeDestruct()
{
	QueueChangedListenerHandle.Unregister();
	CollectionChangedListenerHandle.Unregister();
	Super::NativeDestruct();
}

void UTTDResearchLabWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bShowingResearch)
	{
		return;
	}

	ResearchRefreshAccumulator += InDeltaTime;
	if (ResearchRefreshAccumulator >= 0.5f)
	{
		ResearchRefreshAccumulator = 0.0f;
		RefreshResearchQueueButtons();
	}
}

UTTDResearchSubsystem* UTTDResearchLabWidget::GetResearchSubsystem() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<UTTDResearchSubsystem>() : nullptr;
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
	if (bShowingResearch && !bSuppressQueueChangedRefresh)
	{
		RenderResearch();
	}
}

void UTTDResearchLabWidget::HandleCollectionChangedMessage(FGameplayTag Channel, const FTTDResearchCollectionChangedMessage& Message)
{
	if (!bShowingResearch)
	{
		RenderAlmanac();
	}
}

void UTTDResearchLabWidget::RenderAlmanac()
{
	bShowingResearch = false;
	LastFeedback = FText::GetEmpty();

	if (!ContentBox)
	{
		return;
	}

	ContentBox->ClearChildren();
	QueueSlotButtons.Reset();

	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Title->SetText(FText::FromString(TEXT("研究所 / 图鉴")));
	UseDarkText(Title);
	FSlateFontInfo TitleFont = Title->GetFont();
	TitleFont.Size = 32;
	Title->SetFont(TitleFont);
	ContentBox->AddChildToVerticalBox(Title);

	UHorizontalBox* Tabs = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	ContentBox->AddChildToVerticalBox(Tabs);

	FSimpleDelegate DiagramDelegate;
	DiagramDelegate.BindUObject(this, &UTTDResearchLabWidget::SetCollectionCategory, ETTDCollectionCategory::Diagram);
	Tabs->AddChildToHorizontalBox(MakeButton(WidgetTree, FText::FromString(TEXT("图纸")), true, DiagramDelegate));

	FSimpleDelegate ToyBoxDelegate;
	ToyBoxDelegate.BindUObject(this, &UTTDResearchLabWidget::SetCollectionCategory, ETTDCollectionCategory::ToyBox);
	UHorizontalBoxSlot* ToyBoxTabSlot = Tabs->AddChildToHorizontalBox(MakeButton(WidgetTree, FText::FromString(TEXT("玩具盒")), true, ToyBoxDelegate));
	ToyBoxTabSlot->SetPadding(FMargin(12.0f, 0.0f, 0.0f, 0.0f));

	UScrollBox* ScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
	UVerticalBoxSlot* ScrollSlot = ContentBox->AddChildToVerticalBox(ScrollBox);
	ScrollSlot->SetPadding(FMargin(0.0f, 16.0f, 0.0f, 16.0f));

	UGridPanel* Grid = WidgetTree->ConstructWidget<UGridPanel>(UGridPanel::StaticClass());
	ScrollBox->AddChild(Grid);

	if (UTTDResearchSubsystem* ResearchSubsystem = GetResearchSubsystem())
	{
		const TArray<FTTDCollectionEntry> Entries = ResearchSubsystem->GetCollectionEntries(ActiveCollectionCategory);
		for (int32 Index = 0; Index < Entries.Num(); ++Index)
		{
			const FTTDCollectionEntry Entry = Entries[Index];
			FText ButtonLabel = Entry.bUnlocked ? Entry.DisplayName : FText::FromString(TEXT("未解锁"));
			FSimpleDelegate EntryDelegate;
			EntryDelegate.BindWeakLambda(this, [this, Entry]()
			{
				LastFeedback = DescribeEntry(Entry);
				RenderAlmanac();
			});

			UTTDActionButtonWidget* EntryButton = MakeButton(WidgetTree, ButtonLabel, Entry.bUnlocked, EntryDelegate);
			UGridSlot* GridSlot = Grid->AddChildToGrid(EntryButton, Index / 5, Index % 5);
			GridSlot->SetPadding(FMargin(8.0f));
		}
	}

	FeedbackText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	FeedbackText->SetAutoWrapText(true);
	FeedbackText->SetText(LastFeedback.IsEmpty() ? FText::FromString(TEXT("点击已解锁内容查看说明。")) : LastFeedback);
	UseDarkText(FeedbackText);
	ContentBox->AddChildToVerticalBox(FeedbackText);
}

void UTTDResearchLabWidget::RenderResearch()
{
	bShowingResearch = true;
	ResearchRefreshAccumulator = 0.0f;

	if (!ContentBox)
	{
		return;
	}

	ContentBox->ClearChildren();
	QueueSlotButtons.Reset();

	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Title->SetText(FText::FromString(TEXT("研究所 / 研发")));
	UseDarkText(Title);
	FSlateFontInfo TitleFont = Title->GetFont();
	TitleFont.Size = 32;
	Title->SetFont(TitleFont);
	ContentBox->AddChildToVerticalBox(Title);

	UHorizontalBox* ModeTabs = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	ContentBox->AddChildToVerticalBox(ModeTabs);

	FSimpleDelegate ToyBoxModeDelegate;
	ToyBoxModeDelegate.BindUObject(this, &UTTDResearchLabWidget::RenderResearch);
	ModeTabs->AddChildToHorizontalBox(MakeButton(WidgetTree, FText::FromString(TEXT("玩具盒")), true, ToyBoxModeDelegate));

	FSimpleDelegate DiagramModeDelegate;
	DiagramModeDelegate.BindUObject(this, &UTTDResearchLabWidget::RenderDiagramResearchPlaceholder);
	UHorizontalBoxSlot* DiagramSlot = ModeTabs->AddChildToHorizontalBox(MakeButton(WidgetTree, FText::FromString(TEXT("图纸")), true, DiagramModeDelegate));
	DiagramSlot->SetPadding(FMargin(12.0f, 0.0f, 0.0f, 0.0f));

	UTTDResearchSubsystem* ResearchSubsystem = GetResearchSubsystem();
	if (!ResearchSubsystem)
	{
		LastFeedback = FText::FromString(TEXT("研究所系统未初始化。"));
		return;
	}

	UHorizontalBox* QueueRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UVerticalBoxSlot* QueueSlot = ContentBox->AddChildToVerticalBox(QueueRow);
	QueueSlot->SetPadding(FMargin(0.0f, 16.0f, 0.0f, 16.0f));

	UTextBlock* QueueLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	QueueLabel->SetText(FText::FromString(TEXT("制作队列")));
	UseDarkText(QueueLabel);
	QueueRow->AddChildToHorizontalBox(QueueLabel);

	const TArray<FTTDCraftQueueItem> QueueItems = ResearchSubsystem->GetCraftingQueue();
	for (int32 SlotIndex = 0; SlotIndex < ResearchSubsystem->GetCraftingQueueMaxSlots(); ++SlotIndex)
	{
		FText SlotText = FText::FromString(TEXT("+"));
		bool bSlotEnabled = false;
		FSimpleDelegate SlotDelegate;

		if (QueueItems.IsValidIndex(SlotIndex))
		{
			const FTTDCraftQueueItem QueueItem = QueueItems[SlotIndex];
			SlotText = BuildQueueSlotText(QueueItem);
			bSlotEnabled = QueueItem.bCompleted;
			SlotDelegate.BindWeakLambda(this, [this, QueueItem]()
			{
				ClaimQueueItem(QueueItem.QueueId);
			});
		}

		UTTDActionButtonWidget* QueueButton = MakeButton(WidgetTree, SlotText, bSlotEnabled, SlotDelegate);
		QueueSlotButtons.Add(QueueButton);

		UHorizontalBoxSlot* QueueButtonSlot = QueueRow->AddChildToHorizontalBox(QueueButton);
		QueueButtonSlot->SetPadding(FMargin(12.0f, 0.0f, 0.0f, 0.0f));
	}

	UHorizontalBox* BodyRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	ContentBox->AddChildToVerticalBox(BodyRow);

	UVerticalBox* PreviewBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UHorizontalBoxSlot* PreviewSlot = BodyRow->AddChildToHorizontalBox(PreviewBox);
	PreviewSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	PreviewSlot->SetPadding(FMargin(0.0f, 0.0f, 16.0f, 0.0f));

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
		UTextBlock* SelectedName = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		SelectedName->SetText(SelectedToyBox->DisplayName);
		UseDarkText(SelectedName);
		FSlateFontInfo NameFont = SelectedName->GetFont();
		NameFont.Size = 26;
		SelectedName->SetFont(NameFont);
		PreviewBox->AddChildToVerticalBox(SelectedName);

		UTextBlock* Description = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Description->SetAutoWrapText(true);
		Description->SetText(SelectedToyBox->Description);
		UseDarkText(Description);
		UVerticalBoxSlot* DescriptionSlot = PreviewBox->AddChildToVerticalBox(Description);
		DescriptionSlot->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 12.0f));

		UTextBlock* PartsText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		PartsText->SetText(FText::Format(
			FText::FromString(TEXT("可开出零件：{0}")),
			PartIdsToText(SelectedToyBox->PossiblePartIds)));
		UseDarkText(PartsText);
		PreviewBox->AddChildToVerticalBox(PartsText);

		UTextBlock* DurationText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		DurationText->SetText(FText::Format(
			FText::FromString(TEXT("制作所需时间：{0}")),
			FormatSeconds(SelectedToyBox->CraftDurationSeconds)));
		UseDarkText(DurationText);
		UVerticalBoxSlot* DurationSlot = PreviewBox->AddChildToVerticalBox(DurationText);
		DurationSlot->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 12.0f));

		FSimpleDelegate EnqueueDelegate;
		EnqueueDelegate.BindUObject(this, &UTTDResearchLabWidget::EnqueueSelectedToyBox);
		PreviewBox->AddChildToVerticalBox(MakeButton(WidgetTree, FText::FromString(TEXT("加入制作队列")), true, EnqueueDelegate));
	}

	FeedbackText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	FeedbackText->SetAutoWrapText(true);
	FeedbackText->SetText(LastFeedback.IsEmpty() ? FText::FromString(TEXT("选择右侧玩具盒后可加入制作队列。")) : LastFeedback);
	UseDarkText(FeedbackText);
	UVerticalBoxSlot* FeedbackSlot = PreviewBox->AddChildToVerticalBox(FeedbackText);
	FeedbackSlot->SetPadding(FMargin(0.0f, 16.0f, 0.0f, 0.0f));

	UScrollBox* ToyBoxList = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
	UHorizontalBoxSlot* ListSlot = BodyRow->AddChildToHorizontalBox(ToyBoxList);
	ListSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

	for (const FTTDCollectionEntry& Entry : ResearchSubsystem->GetCollectionEntries(ETTDCollectionCategory::ToyBox))
	{
		FSimpleDelegate SelectDelegate;
		SelectDelegate.BindUObject(this, &UTTDResearchLabWidget::SetSelectedToyBox, Entry.ItemId);

		const FText Label = Entry.bUnlocked
			? Entry.DisplayName
			: FText::Format(FText::FromString(TEXT("{0}\n未解锁")), Entry.DisplayName);

		ToyBoxList->AddChild(MakeButton(WidgetTree, Label, Entry.bUnlocked, SelectDelegate));
	}
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

		if (QueueItems.IsValidIndex(SlotIndex))
		{
			const FTTDCraftQueueItem QueueItem = QueueItems[SlotIndex];
			SlotText = BuildQueueSlotText(QueueItem);
			bSlotEnabled = QueueItem.bCompleted;
		}

		QueueButton->SetLabelAndEnabled(SlotText, bSlotEnabled);
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

void UTTDResearchLabWidget::RenderDiagramResearchPlaceholder()
{
	bShowingResearch = false;
	LastFeedback = FText::FromString(TEXT("图纸研发页签已预留：后续可接入图纸合成、零件消耗与解锁规则。"));

	if (!ContentBox)
	{
		return;
	}

	ContentBox->ClearChildren();

	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Title->SetText(FText::FromString(TEXT("研究所 / 图纸研发")));
	UseDarkText(Title);
	FSlateFontInfo TitleFont = Title->GetFont();
	TitleFont.Size = 32;
	Title->SetFont(TitleFont);
	ContentBox->AddChildToVerticalBox(Title);

	UTextBlock* Description = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Description->SetAutoWrapText(true);
	Description->SetText(LastFeedback);
	UseDarkText(Description);
	UVerticalBoxSlot* DescriptionSlot = ContentBox->AddChildToVerticalBox(Description);
	DescriptionSlot->SetPadding(FMargin(0.0f, 24.0f, 0.0f, 0.0f));
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
			LastFeedback = NewParts.Num() > 0
				? FText::Format(FText::FromString(TEXT("制作完成，新增 {0} 个零件。")), FText::AsNumber(NewParts.Num()))
				: FText::FromString(TEXT("制作完成，获得的零件均已解锁。"));
		}
	}

	RenderResearch();
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
		PartIdsToText(Entry.PartIds));
}

FText UTTDResearchLabWidget::FormatSeconds(const float Seconds) const
{
	const int32 TotalSeconds = FMath::Max(0, FMath::CeilToInt(Seconds));
	const int32 Hours = TotalSeconds / 3600;
	const int32 Minutes = (TotalSeconds % 3600) / 60;
	const int32 RemainingSeconds = TotalSeconds % 60;
	return FText::FromString(FString::Printf(TEXT("%02d:%02d:%02d"), Hours, Minutes, RemainingSeconds));
}
