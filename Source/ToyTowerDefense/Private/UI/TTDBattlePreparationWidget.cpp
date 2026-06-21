#include "UI/TTDBattlePreparationWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "TTDBattleWorldSubsystem.h"
#include "TTDMenuPlayerController.h"
#include "TTDResearchSubsystem.h"
#include "UI/TTDActionButtonWidget.h"
#include "UI/TTDUIDisplayNames.h"
#include "UI/TTDUIStyle.h"

namespace
{
	UBorder* MakeBattlePreparationPanel(UWidgetTree* WidgetTree, const FLinearColor& Color, const FMargin& Padding)
	{
		UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		TTDUIStyle::ApplyPanel(Panel, Color, Padding);
		return Panel;
	}

	UTextBlock* MakeBattlePreparationText(UWidgetTree* WidgetTree, const FText& TextValue, const int32 FontSize, const FLinearColor& Color = TTDUIStyle::Ink(), const ETextJustify::Type Justification = ETextJustify::Left)
	{
		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Text->SetText(TextValue);
		TTDUIStyle::ApplyText(Text, FontSize, Color, Justification, true);
		return Text;
	}

	UTTDActionButtonWidget* MakeBattlePreparationButton(UWidgetTree* WidgetTree, const FText& Label, const bool bEnabled, const FSimpleDelegate& Delegate, const ETTDActionButtonVariant Variant = ETTDActionButtonVariant::Secondary)
	{
		UTTDActionButtonWidget* Button = WidgetTree->ConstructWidget<UTTDActionButtonWidget>(UTTDActionButtonWidget::StaticClass());
		Button->Configure(Label, bEnabled, Delegate, Variant);
		return Button;
	}

	FString JoinPartNames(const UGameInstance* GameInstance, const TArray<FName>& Names)
	{
		return TTDUIDisplayNames::JoinPartIds(GameInstance, Names);
	}

	FText LevelNameText(const FTTDBattleLevelDefinition& LevelDefinition)
	{
		return TTDUIDisplayNames::DisplayNameOrFallback(LevelDefinition.DisplayName, LevelDefinition.LevelId);
	}

	TSet<FName> BuildPreparationRequiredBattleDiagramIds(const UTTDBattleWorldSubsystem* BattleSubsystem)
	{
		TSet<FName> Result;
		if (!BattleSubsystem)
		{
			return Result;
		}

		for (const FTTDBuildingDefinition& BuildingDefinition : BattleSubsystem->GetBuildingDefinitions())
		{
			if (!BuildingDefinition.RequiredDiagramId.IsNone())
			{
				Result.Add(BuildingDefinition.RequiredDiagramId);
			}
		}
		return Result;
	}

	bool IsPreparationUsableBattleDiagram(const FTTDCollectionEntry& Entry, const TSet<FName>& RequiredBattleDiagramIds)
	{
		return Entry.bUnlocked && (RequiredBattleDiagramIds.IsEmpty() || RequiredBattleDiagramIds.Contains(Entry.ItemId));
	}
}

void UTTDBattlePreparationWidget::SetLevelId(const FName InLevelId)
{
	if (LevelId == InLevelId && bDataLoaded)
	{
		if (bLayoutBuilt)
		{
			RebuildOptionLists();
			RefreshSummary();
		}
		return;
	}

	LevelId = InLevelId;
	LoadData();
	if (bLayoutBuilt)
	{
		RebuildOptionLists();
		RefreshSummary();
	}
}

void UTTDBattlePreparationWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	BuildLayout();
	if (bDataLoaded)
	{
		RebuildOptionLists();
		RefreshSummary();
	}
}

void UTTDBattlePreparationWidget::NativeConstruct()
{
	Super::NativeConstruct();
	TTDUIStyle::ResetFadeIn(this, EntranceElapsed);
}

void UTTDBattlePreparationWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	TTDUIStyle::TickFadeIn(this, EntranceElapsed, InDeltaTime);
}

void UTTDBattlePreparationWidget::BuildLayout()
{
	UBorder* Root = MakeBattlePreparationPanel(WidgetTree, TTDUIStyle::Backdrop(), FMargin(32.0f, 28.0f));
	WidgetTree->RootWidget = Root;

	UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Root->AddChild(RootBox);

	TitleText = MakeBattlePreparationText(WidgetTree, FText::FromString(TEXT("关卡准备")), 34, TTDUIStyle::Ink(), ETextJustify::Center);
	UVerticalBoxSlot* TitleSlot = RootBox->AddChildToVerticalBox(TitleText);
	TitleSlot->SetHorizontalAlignment(HAlign_Center);
	TitleSlot->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 18.0f));

	UHorizontalBox* BodyRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UVerticalBoxSlot* BodySlot = RootBox->AddChildToVerticalBox(BodyRow);
	BodySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	UBorder* DiagramPanel = MakeBattlePreparationPanel(WidgetTree, TTDUIStyle::Paper(), FMargin(14.0f));
	UHorizontalBoxSlot* DiagramPanelSlot = BodyRow->AddChildToHorizontalBox(DiagramPanel);
	DiagramPanelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	DiagramPanelSlot->SetPadding(FMargin(0.0f, 0.0f, 12.0f, 0.0f));

	UVerticalBox* DiagramBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	DiagramPanel->AddChild(DiagramBox);
	DiagramCountText = MakeBattlePreparationText(WidgetTree, FText::GetEmpty(), 18);
	DiagramBox->AddChildToVerticalBox(DiagramCountText);
	UScrollBox* DiagramScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
	UVerticalBoxSlot* DiagramScrollSlot = DiagramBox->AddChildToVerticalBox(DiagramScroll);
	DiagramScrollSlot->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 0.0f));
	DiagramScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	DiagramList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	DiagramScroll->AddChild(DiagramList);

	UBorder* ToyBoxPanel = MakeBattlePreparationPanel(WidgetTree, TTDUIStyle::Paper(), FMargin(14.0f));
	UHorizontalBoxSlot* ToyBoxPanelSlot = BodyRow->AddChildToHorizontalBox(ToyBoxPanel);
	ToyBoxPanelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	ToyBoxPanelSlot->SetPadding(FMargin(0.0f, 0.0f, 12.0f, 0.0f));

	UVerticalBox* ToyBoxBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	ToyBoxPanel->AddChild(ToyBoxBox);
	ToyBoxCountText = MakeBattlePreparationText(WidgetTree, FText::GetEmpty(), 18);
	ToyBoxBox->AddChildToVerticalBox(ToyBoxCountText);
	UScrollBox* ToyBoxScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
	UVerticalBoxSlot* ToyBoxScrollSlot = ToyBoxBox->AddChildToVerticalBox(ToyBoxScroll);
	ToyBoxScrollSlot->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 0.0f));
	ToyBoxScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	ToyBoxList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	ToyBoxScroll->AddChild(ToyBoxList);

	UBorder* PreviewPanel = MakeBattlePreparationPanel(WidgetTree, TTDUIStyle::Cream(), FMargin(16.0f));
	UHorizontalBoxSlot* PreviewPanelSlot = BodyRow->AddChildToHorizontalBox(PreviewPanel);
	PreviewPanelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	PreviewText = MakeBattlePreparationText(WidgetTree, FText::FromString(TEXT("点击图纸或玩具盒查看内容。")), 16, TTDUIStyle::MutedInk());
	PreviewPanel->AddChild(PreviewText);

	UHorizontalBox* FooterRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UVerticalBoxSlot* FooterSlot = RootBox->AddChildToVerticalBox(FooterRow);
	FooterSlot->SetPadding(FMargin(0.0f, 18.0f, 0.0f, 0.0f));

	FSimpleDelegate BackDelegate;
	BackDelegate.BindUObject(this, &UTTDBattlePreparationWidget::HandleBackClicked);
	FooterRow->AddChildToHorizontalBox(MakeBattlePreparationButton(WidgetTree, FText::FromString(TEXT("返回选关")), true, BackDelegate, ETTDActionButtonVariant::Danger));

	StatusText = MakeBattlePreparationText(WidgetTree, FText::GetEmpty(), 16, TTDUIStyle::MutedInk(), ETextJustify::Center);
	UHorizontalBoxSlot* StatusSlot = FooterRow->AddChildToHorizontalBox(StatusText);
	StatusSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	StatusSlot->SetPadding(FMargin(18.0f, 0.0f));
	StatusSlot->SetVerticalAlignment(VAlign_Center);

	FSimpleDelegate StartDelegate;
	StartDelegate.BindUObject(this, &UTTDBattlePreparationWidget::HandleStartClicked);
	FooterRow->AddChildToHorizontalBox(MakeBattlePreparationButton(WidgetTree, FText::FromString(TEXT("开始游戏")), true, StartDelegate, ETTDActionButtonVariant::Primary));

	bLayoutBuilt = true;
}

void UTTDBattlePreparationWidget::LoadData()
{
	bDataLoaded = false;
	DiagramEntries.Reset();
	ToyBoxEntries.Reset();
	BaseToyBoxStock.Reset();
	LevelDefinition = FTTDBattleLevelDefinition();

	bool bFoundLevel = false;
	UTTDBattleWorldSubsystem* BattleSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UTTDBattleWorldSubsystem>() : nullptr;
	if (BattleSubsystem)
	{
		for (const FTTDBattleLevelDefinition& Candidate : BattleSubsystem->GetBattleLevelDefinitions())
		{
			if (Candidate.LevelId == LevelId)
			{
				LevelDefinition = Candidate;
				bFoundLevel = true;
				break;
			}
		}
	}
	const TSet<FName> RequiredBattleDiagramIds = BuildPreparationRequiredBattleDiagramIds(BattleSubsystem);

	if (!bFoundLevel)
	{
		SelectedDiagramIds.Reset();
		SelectedToyBoxIds.Reset();
		bDataLoaded = true;
		return;
	}

	if (UTTDResearchSubsystem* ResearchSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UTTDResearchSubsystem>() : nullptr)
	{
		for (const FTTDCollectionEntry& Entry : ResearchSubsystem->GetCollectionEntries(ETTDCollectionCategory::Diagram))
		{
			if (IsPreparationUsableBattleDiagram(Entry, RequiredBattleDiagramIds))
			{
				DiagramEntries.Add(Entry);
			}
		}

		for (const FTTDCollectionEntry& Entry : ResearchSubsystem->GetCollectionEntries(ETTDCollectionCategory::ToyBox))
		{
			if (!Entry.bUnlocked)
			{
				continue;
			}

			if (BattleSubsystem && BattleSubsystem->FindToyBoxDefinition(Entry.ItemId))
			{
				ToyBoxEntries.Add(Entry);
				BaseToyBoxStock.Add(Entry.ItemId, ResearchSubsystem->GetToyBoxCount(Entry.ItemId));
			}
		}
	}

	SelectedDiagramIds.Reset();
	SelectedToyBoxIds.Reset();
	bDataLoaded = true;
}

void UTTDBattlePreparationWidget::RebuildOptionLists()
{
	if (TitleText)
	{
		TitleText->SetText(FText::Format(FText::FromString(TEXT("关卡准备 / {0}")), LevelNameText(LevelDefinition)));
	}

	if (DiagramList)
	{
		DiagramList->ClearChildren();
		for (const FTTDCollectionEntry& Entry : DiagramEntries)
		{
			const bool bSelected = SelectedDiagramIds.Contains(Entry.ItemId);
			const bool bCanSelect = bSelected || SelectedDiagramIds.Num() < FMath::Max(0, LevelDefinition.MaxSelectedDiagrams);
			FSimpleDelegate ToggleDelegate;
			ToggleDelegate.BindWeakLambda(this, [this, Entry]()
			{
				PreviewText->SetText(BuildEntryPreviewText(Entry));
				ToggleDiagram(Entry.ItemId);
			});

			const FText Label = FText::Format(
				FText::FromString(TEXT("{0}{1}\n所需：{2}")),
				bSelected ? FText::FromString(TEXT("[已选] ")) : FText::GetEmpty(),
				Entry.DisplayName,
				FText::FromString(JoinPartNames(GetGameInstance(), Entry.PartIds)));
			UVerticalBoxSlot* EntrySlot = DiagramList->AddChildToVerticalBox(MakeBattlePreparationButton(WidgetTree, Label, bCanSelect, ToggleDelegate));
			EntrySlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
		}
	}

	if (ToyBoxList)
	{
		ToyBoxList->ClearChildren();
		for (const FTTDCollectionEntry& Entry : ToyBoxEntries)
		{
			const int32 SelectedCount = GetSelectedToyBoxCount(Entry.ItemId);
			const int32 AvailableStock = GetAvailableToyBoxStock(Entry.ItemId);
			const bool bCanSelect = SelectedCount > 0 || (AvailableStock > 0 && SelectedToyBoxIds.Num() < FMath::Max(0, LevelDefinition.MaxSelectedToyBoxes));
			FSimpleDelegate ToggleDelegate;
			ToggleDelegate.BindWeakLambda(this, [this, Entry]()
			{
				PreviewText->SetText(BuildEntryPreviewText(Entry));
				ToggleToyBox(Entry.ItemId);
			});

			const FText Label = FText::Format(
				FText::FromString(TEXT("{0}{1}\n已选：{2}  库存：{3}\n可开出：{4}")),
				SelectedCount > 0 ? FText::FromString(TEXT("[已选] ")) : FText::GetEmpty(),
				Entry.DisplayName,
				FText::AsNumber(SelectedCount),
				FText::AsNumber(AvailableStock),
				FText::FromString(JoinPartNames(GetGameInstance(), Entry.PartIds)));
			UVerticalBoxSlot* EntrySlot = ToyBoxList->AddChildToVerticalBox(MakeBattlePreparationButton(WidgetTree, Label, bCanSelect, ToggleDelegate));
			EntrySlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
		}
	}
}

void UTTDBattlePreparationWidget::RefreshSummary()
{
	if (DiagramCountText)
	{
		DiagramCountText->SetText(FText::Format(
			FText::FromString(TEXT("图纸选择：{0}/{1}")),
			FText::AsNumber(SelectedDiagramIds.Num()),
			FText::AsNumber(FMath::Max(0, LevelDefinition.MaxSelectedDiagrams))));
	}

	if (ToyBoxCountText)
	{
		ToyBoxCountText->SetText(FText::Format(
			FText::FromString(TEXT("玩具盒选择：{0}/{1}")),
			FText::AsNumber(SelectedToyBoxIds.Num()),
			FText::AsNumber(FMath::Max(0, LevelDefinition.MaxSelectedToyBoxes))));
	}

	if (StatusText)
	{
		StatusText->SetText(FText::Format(
			FText::FromString(TEXT("已选图纸 {0} 个，玩具盒 {1} 个。")),
			FText::AsNumber(SelectedDiagramIds.Num()),
			FText::AsNumber(SelectedToyBoxIds.Num())));
	}
}

void UTTDBattlePreparationWidget::HandleBackClicked()
{
	if (ATTDMenuPlayerController* Controller = GetOwningPlayer<ATTDMenuPlayerController>())
	{
		Controller->ShowBattleLevelSelect();
	}
}

void UTTDBattlePreparationWidget::HandleStartClicked()
{
	if (SelectedDiagramIds.Num() < 1 || SelectedToyBoxIds.Num() < 1)
	{
		if (StatusText)
		{
			StatusText->SetText(FText::FromString(TEXT("请选择至少 1 张图纸和 1 个玩具盒。")));
			StatusText->SetColorAndOpacity(FSlateColor(TTDUIStyle::Warning()));
		}
		return;
	}

	FTTDBattleLoadout Loadout;
	Loadout.SelectedDiagramIds = SelectedDiagramIds;
	Loadout.SelectedDiagramIds.Sort(
		[](const FName& Left, const FName& Right)
		{
			return Left.LexicalLess(Right);
		});
	TMap<FName, int32> ToyBoxCounts;
	for (const FName ToyBoxId : SelectedToyBoxIds)
	{
		if (!ToyBoxId.IsNone())
		{
			ToyBoxCounts.FindOrAdd(ToyBoxId) += 1;
		}
	}
	for (const TPair<FName, int32>& Pair : ToyBoxCounts)
	{
		Loadout.SelectedToyBoxes.Add(FTTDNameStack(Pair.Key, Pair.Value));
	}
	Loadout.SelectedToyBoxes.Sort(
		[](const FTTDNameStack& Left, const FTTDNameStack& Right)
		{
			return Left.Id.LexicalLess(Right.Id);
		});

	if (ATTDMenuPlayerController* Controller = GetOwningPlayer<ATTDMenuPlayerController>())
	{
		if (!Controller->StartPreparedBattle(LevelId, Loadout) && StatusText)
		{
			StatusText->SetText(Controller->GetLastBattleFailureReason());
		}
	}
}

void UTTDBattlePreparationWidget::ToggleDiagram(const FName DiagramId)
{
	if (SelectedDiagramIds.Contains(DiagramId))
	{
		SelectedDiagramIds.RemoveSingle(DiagramId);
	}
	else if (SelectedDiagramIds.Num() < FMath::Max(0, LevelDefinition.MaxSelectedDiagrams))
	{
		SelectedDiagramIds.Add(DiagramId);
	}

	RebuildOptionLists();
	RefreshSummary();
}

void UTTDBattlePreparationWidget::ToggleToyBox(const FName ToyBoxId)
{
	if (SelectedToyBoxIds.Num() >= FMath::Max(0, LevelDefinition.MaxSelectedToyBoxes) || GetAvailableToyBoxStock(ToyBoxId) <= 0)
	{
		if (SelectedToyBoxIds.Contains(ToyBoxId))
		{
			SelectedToyBoxIds.RemoveSingle(ToyBoxId);
		}
	}
	else
	{
		SelectedToyBoxIds.Add(ToyBoxId);
	}

	RebuildOptionLists();
	RefreshSummary();
}

int32 UTTDBattlePreparationWidget::GetSelectedToyBoxCount(const FName ToyBoxId) const
{
	int32 Count = 0;
	for (const FName SelectedToyBoxId : SelectedToyBoxIds)
	{
		if (SelectedToyBoxId == ToyBoxId)
		{
			++Count;
		}
	}
	return Count;
}

int32 UTTDBattlePreparationWidget::GetAvailableToyBoxStock(const FName ToyBoxId) const
{
	return FMath::Max(0, BaseToyBoxStock.FindRef(ToyBoxId) - GetSelectedToyBoxCount(ToyBoxId));
}

FText UTTDBattlePreparationWidget::BuildEntryPreviewText(const FTTDCollectionEntry& Entry) const
{
	const FText Label = Entry.Category == ETTDCollectionCategory::Diagram
		? FText::FromString(TEXT("组装零件"))
		: FText::FromString(TEXT("可开出零件"));

	return FText::Format(
		FText::FromString(TEXT("{0}\n{1}\n{2}：{3}")),
		Entry.DisplayName,
		Entry.Description,
		Label,
		FText::FromString(JoinPartNames(GetGameInstance(), Entry.PartIds)));
}
