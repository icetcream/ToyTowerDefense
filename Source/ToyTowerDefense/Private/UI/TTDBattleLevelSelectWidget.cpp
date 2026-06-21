#include "UI/TTDBattleLevelSelectWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
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
	constexpr int32 MinVisibleMapNodes = 4;

	UBorder* MakePanel(UWidgetTree* WidgetTree, const FLinearColor& Color, const FMargin& Padding)
	{
		UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		TTDUIStyle::ApplyPanel(Panel, Color, Padding);
		return Panel;
	}

	UTextBlock* MakeText(UWidgetTree* WidgetTree, const FText& TextValue, const int32 FontSize, const FLinearColor& Color = TTDUIStyle::Ink(), const ETextJustify::Type Justification = ETextJustify::Left)
	{
		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Text->SetText(TextValue);
		TTDUIStyle::ApplyText(Text, FontSize, Color, Justification, true);
		return Text;
	}

	UTTDActionButtonWidget* MakeButton(UWidgetTree* WidgetTree, const FText& Label, const bool bEnabled, const FSimpleDelegate& Delegate, const ETTDActionButtonVariant Variant)
	{
		UTTDActionButtonWidget* Button = WidgetTree->ConstructWidget<UTTDActionButtonWidget>(UTTDActionButtonWidget::StaticClass());
		Button->Configure(Label, bEnabled, Delegate, Variant);
		return Button;
	}

	void AddVerticalSpacer(UWidgetTree* WidgetTree, UVerticalBox* Box, const float Height)
	{
		if (!Box)
		{
			return;
		}

		USizeBox* Spacer = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		Spacer->SetHeightOverride(Height);
		Box->AddChildToVerticalBox(Spacer);
	}

	UBorder* MakeTextPanel(UWidgetTree* WidgetTree, const FText& TextValue, const int32 FontSize, const FLinearColor& PanelColor = TTDUIStyle::LightPaper())
	{
		UBorder* Panel = MakePanel(WidgetTree, PanelColor, FMargin(12.0f, 10.0f));
		Panel->AddChild(MakeText(WidgetTree, TextValue, FontSize, TTDUIStyle::Ink()));
		return Panel;
	}

	void AddSectionTitle(UWidgetTree* WidgetTree, UVerticalBox* Box, const FText& Title)
	{
		UTextBlock* Text = MakeText(WidgetTree, Title, 17, TTDUIStyle::Ink());
		UVerticalBoxSlot* Slot = Box->AddChildToVerticalBox(Text);
		Slot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
	}

	void AddCanvasWidget(UCanvasPanel* Canvas, UWidget* Widget, const FVector2D& Anchor, const FVector2D& Size, const FVector2D& Offset = FVector2D::ZeroVector)
	{
		if (!Canvas || !Widget)
		{
		return;
	}

	UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(Widget);
	Slot->SetAnchors(FAnchors(Anchor.X, Anchor.Y, Anchor.X, Anchor.Y));
		Slot->SetAlignment(FVector2D(0.5f, 0.5f));
		Slot->SetPosition(Offset);
		Slot->SetSize(Size);
	}

	FVector2D NodeAnchorForIndex(const int32 Index)
	{
		static const FVector2D Anchors[] = {
			FVector2D(0.24f, 0.38f),
			FVector2D(0.46f, 0.52f),
			FVector2D(0.66f, 0.42f),
			FVector2D(0.78f, 0.62f),
			FVector2D(0.54f, 0.76f),
			FVector2D(0.34f, 0.68f)
		};

		return Anchors[Index % UE_ARRAY_COUNT(Anchors)];
	}

	FText FormatDiagramList(const UGameInstance* GameInstance, const TArray<FName>& Names)
	{
		return FText::FromString(TTDUIDisplayNames::JoinDiagramIds(GameInstance, Names, TEXT("暂无")));
	}

	FText FormatPartList(const UGameInstance* GameInstance, const TArray<FName>& Names)
	{
		return FText::FromString(TTDUIDisplayNames::JoinPartIds(GameInstance, Names, TEXT("暂无")));
	}

	FText FormatPartStacks(const UGameInstance* GameInstance, const TArray<FTTDNameStack>& Stacks)
	{
		return FText::FromString(TTDUIDisplayNames::JoinPartStacks(GameInstance, Stacks, TEXT("暂无")));
	}

	FText FormatToyBoxStacks(const UGameInstance* GameInstance, const TArray<FTTDNameStack>& Stacks)
	{
		return FText::FromString(TTDUIDisplayNames::JoinToyBoxStacks(GameInstance, Stacks, TEXT("暂无")));
	}

	TSet<FName> BuildRequiredBattleDiagramIds(const UTTDBattleWorldSubsystem* BattleSubsystem)
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

	bool IsUsableBattleDiagram(const FTTDCollectionEntry& Entry, const TSet<FName>& RequiredBattleDiagramIds)
	{
		return Entry.bUnlocked && (RequiredBattleDiagramIds.IsEmpty() || RequiredBattleDiagramIds.Contains(Entry.ItemId));
	}
}

void UTTDBattleLevelSelectWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	BuildLayout();
	LoadData();
	RefreshAll();
}

void UTTDBattleLevelSelectWidget::NativeConstruct()
{
	Super::NativeConstruct();
	TTDUIStyle::ResetFadeIn(this, EntranceElapsed);
}

void UTTDBattleLevelSelectWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	TTDUIStyle::TickFadeIn(this, EntranceElapsed, InDeltaTime);
}

void UTTDBattleLevelSelectWidget::BuildLayout()
{
	UBorder* Root = MakePanel(WidgetTree, TTDUIStyle::Backdrop(), FMargin(18.0f));
	WidgetTree->RootWidget = Root;

	UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Root->AddChild(RootBox);

	UBorder* TopBar = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	TTDUIStyle::ApplyTopBarPanel(TopBar);
	UVerticalBoxSlot* TopSlot = RootBox->AddChildToVerticalBox(TopBar);
	TopSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 14.0f));

	UHorizontalBox* TopRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	TopBar->AddChild(TopRow);

	FSimpleDelegate BackDelegate;
	BackDelegate.BindUObject(this, &UTTDBattleLevelSelectWidget::HandleBackClicked);
	UTTDActionButtonWidget* BackButton = MakeButton(WidgetTree, FText::FromString(TEXT("‹")), true, BackDelegate, ETTDActionButtonVariant::Danger);
	UHorizontalBoxSlot* BackSlot = TopRow->AddChildToHorizontalBox(BackButton);
	BackSlot->SetVerticalAlignment(VAlign_Center);

	UTextBlock* Title = MakeText(WidgetTree, FText::FromString(TEXT("玩具工坊防线")), 24, TTDUIStyle::Ink());
	UHorizontalBoxSlot* TitleSlot = TopRow->AddChildToHorizontalBox(Title);
	TitleSlot->SetPadding(FMargin(14.0f, 0.0f, 12.0f, 0.0f));
	TitleSlot->SetVerticalAlignment(VAlign_Center);

	UBorder* ChapterBadge = MakePanel(WidgetTree, FLinearColor(1.0f, 0.91f, 0.67f, 1.0f), FMargin(12.0f, 7.0f));
	ChapterBadge->AddChild(MakeText(WidgetTree, FText::FromString(TEXT("章节 1：玩具桌 1")), 16, TTDUIStyle::Ink(), ETextJustify::Center));
	UHorizontalBoxSlot* ChapterSlot = TopRow->AddChildToHorizontalBox(ChapterBadge);
	ChapterSlot->SetVerticalAlignment(VAlign_Center);

	USizeBox* TopFiller = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	UHorizontalBoxSlot* FillerSlot = TopRow->AddChildToHorizontalBox(TopFiller);
	FillerSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	UBorder* ResourceBadge = MakePanel(WidgetTree, TTDUIStyle::LightPaper(), FMargin(12.0f, 7.0f));
	ToyBoxTotalText = MakeText(WidgetTree, FText::FromString(TEXT("玩具盒总数：0")), 16, TTDUIStyle::Ink(), ETextJustify::Center);
	ResourceBadge->AddChild(ToyBoxTotalText);
	UHorizontalBoxSlot* ResourceSlot = TopRow->AddChildToHorizontalBox(ResourceBadge);
	ResourceSlot->SetVerticalAlignment(VAlign_Center);

	UHorizontalBox* MainRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UVerticalBoxSlot* MainSlot = RootBox->AddChildToVerticalBox(MainRow);
	MainSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	UBorder* MapFrame = MakePanel(WidgetTree, TTDUIStyle::DarkWood(), FMargin(5.0f));
	UHorizontalBoxSlot* MapFrameSlot = MainRow->AddChildToHorizontalBox(MapFrame);
	MapFrameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	MapFrameSlot->SetPadding(FMargin(0.0f, 0.0f, 16.0f, 0.0f));

	UBorder* MapSurface = MakePanel(WidgetTree, FLinearColor(0.83f, 0.52f, 0.27f, 1.0f), FMargin(0.0f));
	MapFrame->AddChild(MapSurface);

	MapCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	MapSurface->AddChild(MapCanvas);

	UBorder* PanelFrame = MakePanel(WidgetTree, TTDUIStyle::DarkWood(), FMargin(5.0f));
	UHorizontalBoxSlot* PanelFrameSlot = MainRow->AddChildToHorizontalBox(PanelFrame);
	PanelFrameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

	USizeBox* PanelSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	PanelSize->SetWidthOverride(430.0f);
	PanelFrame->AddChild(PanelSize);

	UBorder* Panel = MakePanel(WidgetTree, TTDUIStyle::Paper(), FMargin(0.0f));
	PanelSize->AddChild(Panel);

	UVerticalBox* PanelBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Panel->AddChild(PanelBox);

	UBorder* PanelHead = MakePanel(WidgetTree, FLinearColor(0.96f, 0.82f, 0.52f, 1.0f), FMargin(18.0f, 14.0f, 18.0f, 12.0f));
	PanelBox->AddChildToVerticalBox(PanelHead);

	UVerticalBox* HeadBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	PanelHead->AddChild(HeadBox);
	PanelModeText = MakeText(WidgetTree, FText::FromString(TEXT("关卡侦察")), 14, TTDUIStyle::MutedInk());
	HeadBox->AddChildToVerticalBox(PanelModeText);

	UHorizontalBox* StageRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UVerticalBoxSlot* StageRowSlot = HeadBox->AddChildToVerticalBox(StageRow);
	StageRowSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));

	StageNameText = MakeText(WidgetTree, FText::GetEmpty(), 28, TTDUIStyle::Ink());
	UHorizontalBoxSlot* StageNameSlot = StageRow->AddChildToHorizontalBox(StageNameText);
	StageNameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	StageNameSlot->SetVerticalAlignment(VAlign_Center);

	UBorder* StageBadgePanel = MakePanel(WidgetTree, TTDUIStyle::Blue(), FMargin(10.0f, 5.0f));
	StageBadgeText = MakeText(WidgetTree, FText::FromString(TEXT("玩具桌 1")), 13, FLinearColor::White, ETextJustify::Center);
	StageBadgePanel->AddChild(StageBadgeText);
	UHorizontalBoxSlot* StageBadgeSlot = StageRow->AddChildToHorizontalBox(StageBadgePanel);
	StageBadgeSlot->SetVerticalAlignment(VAlign_Center);

	UScrollBox* BodyScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
	UVerticalBoxSlot* BodyScrollSlot = PanelBox->AddChildToVerticalBox(BodyScroll);
	BodyScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	UBorder* BodyPadding = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	BodyPadding->SetBrush(TTDUIStyle::MakeTransparentBrush());
	BodyPadding->SetPadding(FMargin(18.0f, 16.0f));
	BodyScroll->AddChild(BodyPadding);

	PanelBody = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	BodyPadding->AddChild(PanelBody);

	UBorder* FooterPanel = MakePanel(WidgetTree, FLinearColor(0.969f, 0.875f, 0.675f, 1.0f), FMargin(18.0f, 14.0f, 18.0f, 16.0f));
	PanelBox->AddChildToVerticalBox(FooterPanel);

	PanelFooter = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	FooterPanel->AddChild(PanelFooter);
}

void UTTDBattleLevelSelectWidget::LoadData()
{
	CachedLevels.Reset();
	DiagramEntries.Reset();
	ToyBoxEntries.Reset();
	BaseToyBoxStock.Reset();

	UTTDBattleWorldSubsystem* BattleSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UTTDBattleWorldSubsystem>() : nullptr;
	if (BattleSubsystem)
	{
		CachedLevels = BattleSubsystem->GetBattleLevelDefinitions();
	}
	const TSet<FName> RequiredBattleDiagramIds = BuildRequiredBattleDiagramIds(BattleSubsystem);

	if (!CachedLevels.IsEmpty())
	{
		if (SelectedLevelId.IsNone() || !CachedLevels.ContainsByPredicate([this](const FTTDBattleLevelDefinition& Level) { return Level.LevelId == SelectedLevelId; }))
		{
			SelectedLevelId = CachedLevels[0].LevelId;
		}
		SelectLevel(SelectedLevelId);
	}
	else
	{
		SelectedLevelDefinition = FTTDBattleLevelDefinition();
		SelectedLevelId = NAME_None;
	}

	UTTDResearchSubsystem* ResearchSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UTTDResearchSubsystem>() : nullptr;
	if (!ResearchSubsystem)
	{
		return;
	}

	for (const FTTDCollectionEntry& Entry : ResearchSubsystem->GetCollectionEntries(ETTDCollectionCategory::Diagram))
	{
		if (IsUsableBattleDiagram(Entry, RequiredBattleDiagramIds))
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
			// Research save currently stores unlocked toy box types, not owned box stacks.
			// Treat unlocked boxes as temporary loadout stock until a persistent box inventory is added.
			BaseToyBoxStock.Add(Entry.ItemId, FMath::Max(0, SelectedLevelDefinition.MaxSelectedToyBoxes));
		}
	}
}

void UTTDBattleLevelSelectWidget::RefreshAll()
{
	if (ToyBoxTotalText)
	{
		ToyBoxTotalText->SetText(FText::Format(FText::FromString(TEXT("玩具盒总数：{0}")), FText::AsNumber(GetTotalToyBoxStock())));
	}

	RebuildMap();
	RebuildPanel();
}

void UTTDBattleLevelSelectWidget::RebuildMap()
{
	if (!MapCanvas)
	{
		return;
	}

	MapCanvas->ClearChildren();

	UBorder* Label = MakePanel(WidgetTree, FLinearColor(1.0f, 0.976f, 0.914f, 0.78f), FMargin(14.0f, 10.0f));
	Label->AddChild(MakeText(WidgetTree, FText::FromString(TEXT("当前章节选关")), 16, TTDUIStyle::Ink(), ETextJustify::Center));
	AddCanvasWidget(MapCanvas, Label, FVector2D(0.14f, 0.08f), FVector2D(170.0f, 46.0f));

	struct FDecorationSpec
	{
		FVector2D Anchor;
		FVector2D Size;
		const TCHAR* Label;
	};

	const FDecorationSpec Decorations[] = {
		{ FVector2D(0.12f, 0.64f), FVector2D(128.0f, 74.0f), TEXT("积木堆") },
		{ FVector2D(0.72f, 0.22f), FVector2D(146.0f, 86.0f), TEXT("收纳盒") },
		{ FVector2D(0.69f, 0.76f), FVector2D(150.0f, 76.0f), TEXT("轨道区") }
	};

	for (const FDecorationSpec& Decoration : Decorations)
	{
		UBorder* ToyPanel = MakePanel(WidgetTree, FLinearColor(1.0f, 0.976f, 0.914f, 0.46f), FMargin(8.0f));
		ToyPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
		ToyPanel->AddChild(MakeText(WidgetTree, FText::FromString(Decoration.Label), 16, TTDUIStyle::MutedInk(), ETextJustify::Center));
		AddCanvasWidget(MapCanvas, ToyPanel, Decoration.Anchor, Decoration.Size);
	}

	const FVector2D PathAnchors[] = {
		FVector2D(0.35f, 0.45f),
		FVector2D(0.56f, 0.47f),
		FVector2D(0.72f, 0.52f)
	};

	for (int32 Index = 0; Index < UE_ARRAY_COUNT(PathAnchors); ++Index)
	{
		UBorder* PathSegment = MakePanel(WidgetTree, FLinearColor(1.0f, 0.937f, 0.780f, 0.62f), FMargin(0.0f));
		PathSegment->SetVisibility(ESlateVisibility::HitTestInvisible);
		PathSegment->SetRenderTransformAngle(Index == 0 ? 25.0f : (Index == 1 ? -18.0f : 26.0f));
		AddCanvasWidget(MapCanvas, PathSegment, PathAnchors[Index], FVector2D(190.0f, 9.0f));
	}

	const int32 VisibleNodeCount = FMath::Max(MinVisibleMapNodes, CachedLevels.Num());
	for (int32 Index = 0; Index < VisibleNodeCount; ++Index)
	{
		const bool bHasLevel = CachedLevels.IsValidIndex(Index);
		const bool bSelected = bHasLevel && CachedLevels[Index].LevelId == SelectedLevelId;
		const FText LabelText = bHasLevel
			? FText::AsNumber(Index + 1)
			: FText::FromString(TEXT("锁"));
		const ETTDActionButtonVariant Variant = !bHasLevel
			? ETTDActionButtonVariant::LevelNodeLocked
			: (bSelected ? ETTDActionButtonVariant::LevelNodeSelected : ETTDActionButtonVariant::LevelNode);

		FSimpleDelegate NodeDelegate;
		if (bHasLevel)
		{
			const FName LevelId = CachedLevels[Index].LevelId;
			NodeDelegate.BindWeakLambda(this, [this, LevelId]()
			{
				HandleLevelNodeClicked(LevelId);
			});
		}

		UTTDActionButtonWidget* NodeButton = MakeButton(WidgetTree, LabelText, bHasLevel, NodeDelegate, Variant);
		AddCanvasWidget(MapCanvas, NodeButton, NodeAnchorForIndex(Index), FVector2D(74.0f, 58.0f));
	}
}

void UTTDBattleLevelSelectWidget::RebuildPanel()
{
	if (StageNameText)
	{
		StageNameText->SetText(SelectedLevelId.IsNone() ? FText::FromString(TEXT("未配置关卡")) : GetLevelDisplayName(SelectedLevelDefinition));
	}

	if (StageBadgeText)
	{
		StageBadgeText->SetText(SelectedLevelId.IsNone() ? FText::FromString(TEXT("未开放")) : FText::FromString(TEXT("玩具桌 1")));
	}

	if (PanelMode == EPanelMode::Scout)
	{
		RenderScoutPanel();
	}
	else
	{
		RenderLoadoutPanel();
	}
}

void UTTDBattleLevelSelectWidget::RenderScoutPanel()
{
	if (!PanelBody || !PanelFooter)
	{
		return;
	}

	PanelBody->ClearChildren();
	PanelFooter->ClearChildren();
	if (PanelModeText)
	{
		PanelModeText->SetText(FText::FromString(TEXT("关卡侦察")));
	}

	if (CachedLevels.IsEmpty())
	{
		PanelBody->AddChildToVerticalBox(MakeTextPanel(WidgetTree, FText::FromString(TEXT("没有配置可用关卡。")), 17));
		return;
	}

	AddSectionTitle(WidgetTree, PanelBody, FText::FromString(TEXT("玩具桌预览")));
	USizeBox* PreviewSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	PreviewSize->SetHeightOverride(130.0f);
	UBorder* PreviewPanel = MakePanel(WidgetTree, TTDUIStyle::Wood(), FMargin(12.0f));
	PreviewPanel->AddChild(MakeText(WidgetTree, GetLevelDisplayName(SelectedLevelDefinition), 28, TTDUIStyle::Cream(), ETextJustify::Center));
	PreviewSize->AddChild(PreviewPanel);
	UVerticalBoxSlot* PreviewSlot = PanelBody->AddChildToVerticalBox(PreviewSize);
	PreviewSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));

	AddSectionTitle(WidgetTree, PanelBody, FText::FromString(TEXT("关卡信息")));
	UUniformGridPanel* InfoGrid = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass());
	const FText InfoTexts[] = {
		FText::Format(FText::FromString(TEXT("目标\n守住城堡并清空 {0} 波敌人")), FText::AsNumber(SelectedLevelDefinition.WaveIds.Num())),
		FText::FromString(TEXT("难度\n暂未配置，显示标准")),
		FText::FromString(TEXT("地图\n玩具桌 1")),
		BuildLevelInfoText(SelectedLevelDefinition)
	};

	for (int32 Index = 0; Index < UE_ARRAY_COUNT(InfoTexts); ++Index)
	{
		UBorder* Card = MakeTextPanel(WidgetTree, InfoTexts[Index], 15);
		UUniformGridSlot* CardSlot = InfoGrid->AddChildToUniformGrid(Card, Index / 2, Index % 2);
		CardSlot->SetHorizontalAlignment(HAlign_Fill);
		CardSlot->SetVerticalAlignment(VAlign_Fill);
	}
	UVerticalBoxSlot* InfoSlot = PanelBody->AddChildToVerticalBox(InfoGrid);
	InfoSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));

	AddSectionTitle(WidgetTree, PanelBody, FText::FromString(TEXT("关卡说明")));
	UVerticalBoxSlot* DescriptionSlot = PanelBody->AddChildToVerticalBox(MakeTextPanel(
		WidgetTree,
		SelectedLevelDefinition.Description.IsEmpty() ? FText::FromString(TEXT("暂无关卡说明。")) : SelectedLevelDefinition.Description,
		15));
	DescriptionSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));

	AddSectionTitle(WidgetTree, PanelBody, FText::FromString(TEXT("奖励预览")));
	PanelBody->AddChildToVerticalBox(MakeTextPanel(WidgetTree, BuildRewardPreviewText(SelectedLevelDefinition), 15));

	FSimpleDelegate PrepareDelegate;
	PrepareDelegate.BindUObject(this, &UTTDBattleLevelSelectWidget::HandlePrepareClicked);
	PanelFooter->AddChildToVerticalBox(MakeButton(WidgetTree, FText::FromString(TEXT("准备出战")), !SelectedLevelId.IsNone(), PrepareDelegate, ETTDActionButtonVariant::Primary));
}

void UTTDBattleLevelSelectWidget::RenderLoadoutPanel()
{
	if (!PanelBody || !PanelFooter)
	{
		return;
	}

	PanelBody->ClearChildren();
	PanelFooter->ClearChildren();
	if (PanelModeText)
	{
		PanelModeText->SetText(FText::FromString(TEXT("战前配置")));
	}

	UHorizontalBox* ActionRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	FSimpleDelegate BackScoutDelegate;
	BackScoutDelegate.BindUObject(this, &UTTDBattleLevelSelectWidget::HandleBackToScoutClicked);
	UHorizontalBoxSlot* BackScoutSlot = ActionRow->AddChildToHorizontalBox(MakeButton(WidgetTree, FText::FromString(TEXT("返回侦察")), true, BackScoutDelegate, ETTDActionButtonVariant::Ghost));
	BackScoutSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));

	FSimpleDelegate PresetDelegate;
	PresetDelegate.BindUObject(this, &UTTDBattleLevelSelectWidget::HandleUsePresetClicked);
	ActionRow->AddChildToHorizontalBox(MakeButton(WidgetTree, FText::FromString(TEXT("使用预设")), true, PresetDelegate, ETTDActionButtonVariant::Ghost));
	PanelBody->AddChildToVerticalBox(ActionRow);

	UVerticalBoxSlot* RequirementSlot = PanelBody->AddChildToVerticalBox(MakeTextPanel(WidgetTree, FText::FromString(TEXT("最低要求：至少 1 张图纸、1 个玩具盒")), 14, FLinearColor(1.0f, 0.976f, 0.914f, 1.0f)));
	RequirementSlot->SetPadding(FMargin(0.0f, 10.0f, 0.0f, 14.0f));

	AddSectionTitle(WidgetTree, PanelBody, FText::Format(
		FText::FromString(TEXT("已选图纸 {0}/{1}")),
		FText::AsNumber(SelectedDiagramIds.Num()),
		FText::AsNumber(FMath::Max(0, SelectedLevelDefinition.MaxSelectedDiagrams))));

	UUniformGridPanel* DiagramSlots = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass());
	for (int32 SlotIndex = 0; SlotIndex < FMath::Max(0, SelectedLevelDefinition.MaxSelectedDiagrams); ++SlotIndex)
	{
		const bool bFilled = SelectedDiagramIds.IsValidIndex(SlotIndex);
		FText SlotText = FText::FromString(TEXT("空"));
		if (bFilled)
		{
			const FTTDCollectionEntry* Entry = FindDiagramEntry(SelectedDiagramIds[SlotIndex]);
			SlotText = Entry ? Entry->DisplayName : FText::FromName(SelectedDiagramIds[SlotIndex]);
		}

		FSimpleDelegate RemoveDelegate;
		if (bFilled)
		{
			RemoveDelegate.BindWeakLambda(this, [this, SlotIndex]()
			{
				RemoveDiagramAt(SlotIndex);
			});
		}

		UUniformGridSlot* DiagramSlot = DiagramSlots->AddChildToUniformGrid(
			MakeButton(WidgetTree, SlotText, bFilled, RemoveDelegate, bFilled ? ETTDActionButtonVariant::FilledSlot : ETTDActionButtonVariant::EmptySlot),
			0,
			SlotIndex);
		DiagramSlot->SetHorizontalAlignment(HAlign_Fill);
	}
	UVerticalBoxSlot* DiagramSlotsBoxSlot = PanelBody->AddChildToVerticalBox(DiagramSlots);
	DiagramSlotsBoxSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 14.0f));

	AddSectionTitle(WidgetTree, PanelBody, FText::FromString(TEXT("图纸库")));
	UHorizontalBox* TabRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	const FName Tabs[] = { TEXT("建筑"), TEXT("单位"), TEXT("事件") };
	for (const FName TabId : Tabs)
	{
		FSimpleDelegate TabDelegate;
		TabDelegate.BindWeakLambda(this, [this, TabId]()
		{
			HandleDiagramTabClicked(TabId);
		});
		UHorizontalBoxSlot* TabSlot = TabRow->AddChildToHorizontalBox(MakeButton(
			WidgetTree,
			FText::FromName(TabId),
			true,
			TabDelegate,
			ActiveDiagramTab == TabId ? ETTDActionButtonVariant::LibraryCardSelected : ETTDActionButtonVariant::LibraryCard));
		TabSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		TabSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
	}
	PanelBody->AddChildToVerticalBox(TabRow);

	UUniformGridPanel* DiagramCards = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass());
	if (ActiveDiagramTab == TEXT("建筑"))
	{
		for (int32 Index = 0; Index < DiagramEntries.Num(); ++Index)
		{
			const FTTDCollectionEntry& Entry = DiagramEntries[Index];
			const bool bSelected = SelectedDiagramIds.Contains(Entry.ItemId);
			const bool bCanSelect = bSelected || SelectedDiagramIds.Num() < FMath::Max(0, SelectedLevelDefinition.MaxSelectedDiagrams);
			FSimpleDelegate DiagramDelegate;
			DiagramDelegate.BindWeakLambda(this, [this, Entry]()
			{
				ToggleDiagram(Entry.ItemId);
			});
			const FText Label = FText::Format(
				FText::FromString(TEXT("{0}\n{1}")),
				Entry.DisplayName,
				BuildEntryPartsText(Entry));
			UUniformGridSlot* CardSlot = DiagramCards->AddChildToUniformGrid(
				MakeButton(WidgetTree, Label, bCanSelect, DiagramDelegate, bSelected ? ETTDActionButtonVariant::LibraryCardSelected : ETTDActionButtonVariant::LibraryCard),
				Index / 3,
				Index % 3);
			CardSlot->SetHorizontalAlignment(HAlign_Fill);
		}
	}
	else
	{
		// Diagram categories are not modeled in battle DataTables yet.
		DiagramCards->AddChildToUniformGrid(MakeTextPanel(WidgetTree, FText::FromString(TEXT("该分类暂无配置数据。")), 14), 0, 0);
	}
	UVerticalBoxSlot* DiagramCardsSlot = PanelBody->AddChildToVerticalBox(DiagramCards);
	DiagramCardsSlot->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 16.0f));

	AddSectionTitle(WidgetTree, PanelBody, FText::Format(
		FText::FromString(TEXT("已选玩具盒 {0}/{1}")),
		FText::AsNumber(SelectedToyBoxIds.Num()),
		FText::AsNumber(FMath::Max(0, SelectedLevelDefinition.MaxSelectedToyBoxes))));

	UUniformGridPanel* ToyBoxSlots = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass());
	for (int32 SlotIndex = 0; SlotIndex < FMath::Max(0, SelectedLevelDefinition.MaxSelectedToyBoxes); ++SlotIndex)
	{
		const bool bFilled = SelectedToyBoxIds.IsValidIndex(SlotIndex);
		FText SlotText = FText::FromString(TEXT("空"));
		if (bFilled)
		{
			const FTTDCollectionEntry* Entry = FindToyBoxEntry(SelectedToyBoxIds[SlotIndex]);
			SlotText = Entry ? Entry->DisplayName : FText::FromName(SelectedToyBoxIds[SlotIndex]);
		}

		FSimpleDelegate RemoveDelegate;
		if (bFilled)
		{
			RemoveDelegate.BindWeakLambda(this, [this, SlotIndex]()
			{
				RemoveToyBoxAt(SlotIndex);
			});
		}

		UUniformGridSlot* ToyBoxSlot = ToyBoxSlots->AddChildToUniformGrid(
			MakeButton(WidgetTree, SlotText, bFilled, RemoveDelegate, bFilled ? ETTDActionButtonVariant::FilledSlot : ETTDActionButtonVariant::EmptySlot),
			0,
			SlotIndex);
		ToyBoxSlot->SetHorizontalAlignment(HAlign_Fill);
	}
	UVerticalBoxSlot* ToyBoxSlotsBoxSlot = PanelBody->AddChildToVerticalBox(ToyBoxSlots);
	ToyBoxSlotsBoxSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 14.0f));

	AddSectionTitle(WidgetTree, PanelBody, FText::FromString(TEXT("玩具盒库存")));
	UUniformGridPanel* ToyBoxCards = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass());
	for (int32 Index = 0; Index < ToyBoxEntries.Num(); ++Index)
	{
		const FTTDCollectionEntry& Entry = ToyBoxEntries[Index];
		const int32 AvailableStock = GetAvailableToyBoxStock(Entry.ItemId);
		const bool bSelected = GetSelectedToyBoxCount(Entry.ItemId) > 0;
		const bool bCanSelect = AvailableStock > 0 && SelectedToyBoxIds.Num() < FMath::Max(0, SelectedLevelDefinition.MaxSelectedToyBoxes);
		FSimpleDelegate ToyBoxDelegate;
		ToyBoxDelegate.BindWeakLambda(this, [this, Entry]()
		{
			PickToyBox(Entry.ItemId);
		});

		const FText Label = FText::Format(
			FText::FromString(TEXT("{0}\n库存 {1}\n{2}")),
			Entry.DisplayName,
			FText::AsNumber(AvailableStock),
			BuildToyBoxCardText(Entry));
		UUniformGridSlot* CardSlot = ToyBoxCards->AddChildToUniformGrid(
			MakeButton(WidgetTree, Label, bCanSelect, ToyBoxDelegate, bSelected ? ETTDActionButtonVariant::LibraryCardSelected : ETTDActionButtonVariant::LibraryCard),
			Index / 3,
			Index % 3);
		CardSlot->SetHorizontalAlignment(HAlign_Fill);
	}
	PanelBody->AddChildToVerticalBox(ToyBoxCards);

	StatusText = MakeText(
		WidgetTree,
		CanStartBattle() ? FText::FromString(TEXT("配置已满足，可以开始战斗。")) : FText::FromString(TEXT("请选择至少 1 张图纸和 1 个玩具盒。")),
		15,
		CanStartBattle() ? TTDUIStyle::Success() : TTDUIStyle::MutedInk(),
		ETextJustify::Center);
	UVerticalBoxSlot* StatusSlot = PanelFooter->AddChildToVerticalBox(StatusText);
	StatusSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));

	FSimpleDelegate StartDelegate;
	StartDelegate.BindUObject(this, &UTTDBattleLevelSelectWidget::HandleStartBattleClicked);
	PanelFooter->AddChildToVerticalBox(MakeButton(WidgetTree, FText::FromString(TEXT("开始战斗")), CanStartBattle(), StartDelegate, ETTDActionButtonVariant::Primary));
}

void UTTDBattleLevelSelectWidget::ResetTemporaryLoadout()
{
	SelectedDiagramIds.Reset();
	SelectedToyBoxIds.Reset();
	ActiveDiagramTab = TEXT("建筑");
}

void UTTDBattleLevelSelectWidget::HandleBackClicked()
{
	if (ATTDMenuPlayerController* Controller = GetOwningPlayer<ATTDMenuPlayerController>())
	{
		Controller->ShowMainScreen();
	}
}

void UTTDBattleLevelSelectWidget::HandleLevelNodeClicked(const FName LevelId)
{
	if (SelectLevel(LevelId))
	{
		PanelMode = EPanelMode::Scout;
		ResetTemporaryLoadout();
		LoadData();
		RefreshAll();
	}
}

void UTTDBattleLevelSelectWidget::HandlePrepareClicked()
{
	PanelMode = EPanelMode::Loadout;
	ResetTemporaryLoadout();
	RebuildPanel();
}

void UTTDBattleLevelSelectWidget::HandleBackToScoutClicked()
{
	PanelMode = EPanelMode::Scout;
	ResetTemporaryLoadout();
	RebuildPanel();
}

void UTTDBattleLevelSelectWidget::HandleUsePresetClicked()
{
	ResetTemporaryLoadout();
	for (const FTTDCollectionEntry& Entry : DiagramEntries)
	{
		if (SelectedDiagramIds.Num() >= FMath::Max(0, SelectedLevelDefinition.MaxSelectedDiagrams))
		{
			break;
		}
		SelectedDiagramIds.Add(Entry.ItemId);
	}

	for (const FTTDCollectionEntry& Entry : ToyBoxEntries)
	{
		while (SelectedToyBoxIds.Num() < FMath::Max(0, SelectedLevelDefinition.MaxSelectedToyBoxes)
			&& GetAvailableToyBoxStock(Entry.ItemId) > 0)
		{
			SelectedToyBoxIds.Add(Entry.ItemId);
		}

		if (SelectedToyBoxIds.Num() >= FMath::Max(0, SelectedLevelDefinition.MaxSelectedToyBoxes))
		{
			break;
		}
	}

	RebuildPanel();
}

void UTTDBattleLevelSelectWidget::HandleStartBattleClicked()
{
	if (!CanStartBattle())
	{
		if (StatusText)
		{
			StatusText->SetText(FText::FromString(TEXT("请选择至少 1 张图纸和 1 个玩具盒。")));
			StatusText->SetColorAndOpacity(FSlateColor(TTDUIStyle::Warning()));
		}
		return;
	}

	if (ATTDMenuPlayerController* Controller = GetOwningPlayer<ATTDMenuPlayerController>())
	{
		if (!Controller->StartPreparedBattle(SelectedLevelId, BuildLoadout()) && StatusText)
		{
			StatusText->SetText(Controller->GetLastBattleFailureReason());
			StatusText->SetColorAndOpacity(FSlateColor(TTDUIStyle::Danger()));
		}
	}
}

void UTTDBattleLevelSelectWidget::HandleDiagramTabClicked(const FName TabId)
{
	ActiveDiagramTab = TabId;
	RebuildPanel();
}

void UTTDBattleLevelSelectWidget::ToggleDiagram(const FName DiagramId)
{
	if (DiagramId.IsNone())
	{
		return;
	}

	const int32 ExistingIndex = SelectedDiagramIds.Find(DiagramId);
	if (ExistingIndex != INDEX_NONE)
	{
		SelectedDiagramIds.RemoveAt(ExistingIndex);
	}
	else if (SelectedDiagramIds.Num() < FMath::Max(0, SelectedLevelDefinition.MaxSelectedDiagrams))
	{
		SelectedDiagramIds.Add(DiagramId);
	}

	RebuildPanel();
}

void UTTDBattleLevelSelectWidget::PickToyBox(const FName ToyBoxId)
{
	if (ToyBoxId.IsNone())
	{
		return;
	}

	if (SelectedToyBoxIds.Num() >= FMath::Max(0, SelectedLevelDefinition.MaxSelectedToyBoxes)
		|| GetAvailableToyBoxStock(ToyBoxId) <= 0)
	{
		return;
	}

	SelectedToyBoxIds.Add(ToyBoxId);
	RebuildPanel();
}

void UTTDBattleLevelSelectWidget::RemoveDiagramAt(const int32 Index)
{
	if (SelectedDiagramIds.IsValidIndex(Index))
	{
		SelectedDiagramIds.RemoveAt(Index);
		RebuildPanel();
	}
}

void UTTDBattleLevelSelectWidget::RemoveToyBoxAt(const int32 Index)
{
	if (SelectedToyBoxIds.IsValidIndex(Index))
	{
		SelectedToyBoxIds.RemoveAt(Index);
		RebuildPanel();
	}
}

bool UTTDBattleLevelSelectWidget::SelectLevel(const FName LevelId)
{
	const FTTDBattleLevelDefinition* FoundLevel = CachedLevels.FindByPredicate(
		[LevelId](const FTTDBattleLevelDefinition& LevelDefinition)
		{
			return LevelDefinition.LevelId == LevelId;
		});

	if (!FoundLevel)
	{
		return false;
	}

	SelectedLevelId = LevelId;
	SelectedLevelDefinition = *FoundLevel;
	return true;
}

bool UTTDBattleLevelSelectWidget::CanStartBattle() const
{
	return !SelectedLevelId.IsNone() && SelectedDiagramIds.Num() >= 1 && SelectedToyBoxIds.Num() >= 1;
}

int32 UTTDBattleLevelSelectWidget::GetSelectedToyBoxCount(const FName ToyBoxId) const
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

int32 UTTDBattleLevelSelectWidget::GetAvailableToyBoxStock(const FName ToyBoxId) const
{
	return FMath::Max(0, BaseToyBoxStock.FindRef(ToyBoxId) - GetSelectedToyBoxCount(ToyBoxId));
}

int32 UTTDBattleLevelSelectWidget::GetTotalToyBoxStock() const
{
	int32 Total = 0;
	for (const TPair<FName, int32>& Pair : BaseToyBoxStock)
	{
		Total += FMath::Max(0, Pair.Value);
	}
	return Total;
}

FTTDBattleLoadout UTTDBattleLevelSelectWidget::BuildLoadout() const
{
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
	return Loadout;
}

FText UTTDBattleLevelSelectWidget::GetLevelDisplayName(const FTTDBattleLevelDefinition& LevelDefinition) const
{
	return TTDUIDisplayNames::DisplayNameOrFallback(LevelDefinition.DisplayName, LevelDefinition.LevelId);
}

FText UTTDBattleLevelSelectWidget::BuildRewardPreviewText(const FTTDBattleLevelDefinition& LevelDefinition) const
{
	return FText::Format(
		FText::FromString(TEXT("初始图纸：{0}\n初始玩具盒：{1}\n初始零件：{2}\n初始金币：{3}")),
		FormatDiagramList(GetGameInstance(), LevelDefinition.StartingDiagramIds),
		FormatToyBoxStacks(GetGameInstance(), LevelDefinition.StartingToyBoxes),
		FormatPartStacks(GetGameInstance(), LevelDefinition.StartingParts),
		FText::AsNumber(LevelDefinition.StartingCurrency));
}

FText UTTDBattleLevelSelectWidget::BuildLevelInfoText(const FTTDBattleLevelDefinition& LevelDefinition) const
{
	return FText::Format(
		FText::FromString(TEXT("怪物\n{0} 波，情报表待配置")),
		FText::AsNumber(LevelDefinition.WaveIds.Num()));
}

FText UTTDBattleLevelSelectWidget::BuildEntryPartsText(const FTTDCollectionEntry& Entry) const
{
	return FText::Format(FText::FromString(TEXT("零件：{0}")), FormatPartList(GetGameInstance(), Entry.PartIds));
}

FText UTTDBattleLevelSelectWidget::BuildToyBoxCardText(const FTTDCollectionEntry& Entry) const
{
	return FText::Format(FText::FromString(TEXT("可开：{0}")), FormatPartList(GetGameInstance(), Entry.PartIds));
}

const FTTDCollectionEntry* UTTDBattleLevelSelectWidget::FindDiagramEntry(const FName DiagramId) const
{
	return DiagramEntries.FindByPredicate(
		[DiagramId](const FTTDCollectionEntry& Entry)
		{
			return Entry.ItemId == DiagramId;
		});
}

const FTTDCollectionEntry* UTTDBattleLevelSelectWidget::FindToyBoxEntry(const FName ToyBoxId) const
{
	return ToyBoxEntries.FindByPredicate(
		[ToyBoxId](const FTTDCollectionEntry& Entry)
		{
			return Entry.ItemId == ToyBoxId;
		});
}
