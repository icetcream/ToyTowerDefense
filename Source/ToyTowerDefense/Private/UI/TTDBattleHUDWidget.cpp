#include "UI/TTDBattleHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "TTDDeveloperSettings.h"
#include "TTDBattleWorldSubsystem.h"
#include "TTDMenuPlayerController.h"
#include "UI/TTDActionButtonWidget.h"
#include "UI/TTDUIDisplayNames.h"
#include "UI/TTDUIStyle.h"

namespace
{
	UTextBlock* MakeBattleHUDText(UWidgetTree* WidgetTree, const FText& TextValue, const int32 FontSize, const FLinearColor& Color = TTDUIStyle::Ink(), const ETextJustify::Type Justification = ETextJustify::Left)
	{
		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Text->SetText(TextValue);
		TTDUIStyle::ApplyText(Text, FontSize, Color, Justification, true);
		return Text;
	}

	UBorder* MakeBattleHUDPanel(UWidgetTree* WidgetTree, const FLinearColor& Color, const FMargin& Padding)
	{
		UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		TTDUIStyle::ApplyPanel(Panel, Color, Padding);
		Panel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		return Panel;
	}

	FText GetBuildingDisplayName(const FTTDBuildingDefinition& BuildingDefinition)
	{
		return TTDUIDisplayNames::DisplayNameOrFallback(BuildingDefinition.DisplayName, BuildingDefinition.BuildingId);
	}

	FText BuildBuildingCostText(const UGameInstance* GameInstance, const FTTDBuildingDefinition& BuildingDefinition)
	{
		return FText::Format(
			FText::FromString(TEXT("消耗：{0}")),
			FText::FromString(TTDUIDisplayNames::JoinPartStacks(GameInstance, BuildingDefinition.PartCosts)));
	}

	FText BuildBuildingHoverText(const UGameInstance* GameInstance, const FTTDBuildingDefinition& BuildingDefinition)
	{
		return FText::Format(
			FText::FromString(TEXT("{0}\n需要图纸：{1}\n建造消耗：{2}")),
			GetBuildingDisplayName(BuildingDefinition),
			BuildingDefinition.RequiredDiagramId.IsNone()
				? FText::FromString(TEXT("无"))
				: TTDUIDisplayNames::DiagramName(GameInstance, BuildingDefinition.RequiredDiagramId),
			FText::FromString(TTDUIDisplayNames::JoinPartStacks(GameInstance, BuildingDefinition.PartCosts)));
	}

	int32 FindBattleHUDStackCount(const TArray<FTTDNameStack>& Stacks, const FName Id)
	{
		for (const FTTDNameStack& Stack : Stacks)
		{
			if (Stack.Id == Id)
			{
				return FMath::Max(0, Stack.Count);
			}
		}
		return 0;
	}

	FText BattleStateToText(const ETTDBattleState State)
	{
		switch (State)
		{
		case ETTDBattleState::Running:
			return FText::FromString(TEXT("进行中"));
		case ETTDBattleState::Victory:
			return FText::FromString(TEXT("胜利"));
		case ETTDBattleState::Defeat:
			return FText::FromString(TEXT("失败"));
		case ETTDBattleState::ConfigurationError:
			return FText::FromString(TEXT("配置错误"));
		case ETTDBattleState::Inactive:
		default:
			return FText::FromString(TEXT("未开始"));
		}
	}

	FText BattlePhaseToText(const ETTDBattlePhase Phase)
	{
		switch (Phase)
		{
		case ETTDBattlePhase::Preparing:
			return FText::FromString(TEXT("准备"));
		case ETTDBattlePhase::Buffer:
			return FText::FromString(TEXT("缓冲"));
		case ETTDBattlePhase::WaveActive:
			return FText::FromString(TEXT("战斗"));
		case ETTDBattlePhase::VictoryDelay:
			return FText::FromString(TEXT("胜利"));
		case ETTDBattlePhase::Defeat:
			return FText::FromString(TEXT("失败"));
		case ETTDBattlePhase::ConfigurationError:
			return FText::FromString(TEXT("配置错误"));
		case ETTDBattlePhase::None:
		default:
			return FText::FromString(TEXT("未开始"));
		}
	}
}

void UTTDBattleHUDWidget::SetStatusMessage(const FText& Message)
{
	if (StatusText)
	{
		StatusText->SetText(Message);
	}
}

void UTTDBattleHUDWidget::RefreshSelectedBuilding()
{
	const UTTDBattleWorldSubsystem* BattleSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UTTDBattleWorldSubsystem>() : nullptr;
	if (!BattleSubsystem || !SelectedText)
	{
		return;
	}

	const ATTDMenuPlayerController* Controller = GetOwningPlayer<ATTDMenuPlayerController>();
	const FName SelectedBuildingId = Controller ? Controller->GetSelectedBattleBuildingId() : NAME_None;
	const FTTDBuildingDefinition* SelectedBuildingDefinition = BattleSubsystem->FindBuildingDefinition(SelectedBuildingId);
	SelectedText->SetText(SelectedBuildingId.IsNone()
		? FText::FromString(TEXT("当前选择：未选择"))
		: FText::Format(
			FText::FromString(TEXT("当前选择：{0}")),
			SelectedBuildingDefinition ? GetBuildingDisplayName(*SelectedBuildingDefinition) : TTDUIDisplayNames::KnownId(SelectedBuildingId)));
}

void UTTDBattleHUDWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	BuildLayout();
	BuildActionLists();
	RefreshFromBattleSubsystem();
}

void UTTDBattleHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	TTDUIStyle::ResetFadeIn(this, EntranceElapsed);
	RegisterMessageListeners();
	RefreshFromBattleSubsystem();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			TimedRefreshTimerHandle,
			this,
			&UTTDBattleHUDWidget::RefreshTimedTextsFromTimer,
			1.0f,
			true);
	}
}

void UTTDBattleHUDWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TimedRefreshTimerHandle);
	}
	UnregisterMessageListeners();
	Super::NativeDestruct();
}

void UTTDBattleHUDWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	TTDUIStyle::TickFadeIn(this, EntranceElapsed, InDeltaTime);
}

void UTTDBattleHUDWidget::BuildLayout()
{
	UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	RootOverlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	WidgetTree->RootWidget = RootOverlay;

	UBorder* TopBar = MakeBattleHUDPanel(WidgetTree, FLinearColor(0.98f, 0.91f, 0.74f, 0.92f), FMargin(18.0f, 10.0f));
	UOverlaySlot* TopBarSlot = RootOverlay->AddChildToOverlay(TopBar);
	TopBarSlot->SetHorizontalAlignment(HAlign_Fill);
	TopBarSlot->SetVerticalAlignment(VAlign_Top);
	TopBarSlot->SetPadding(FMargin(16.0f, 14.0f, 16.0f, 0.0f));

	UHorizontalBox* TopRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	TopBar->AddChild(TopRow);

	StateText = MakeBattleHUDText(WidgetTree, FText::GetEmpty(), 18);
	PhaseText = MakeBattleHUDText(WidgetTree, FText::GetEmpty(), 16);
	WaveText = MakeBattleHUDText(WidgetTree, FText::GetEmpty(), 16);
	ProgressText = MakeBattleHUDText(WidgetTree, FText::GetEmpty(), 16);
	CastleText = MakeBattleHUDText(WidgetTree, FText::GetEmpty(), 16);
	CurrencyText = MakeBattleHUDText(WidgetTree, FText::GetEmpty(), 16);

	TArray<UTextBlock*> TopTexts = {
		StateText.Get(),
		PhaseText.Get(),
		WaveText.Get(),
		ProgressText.Get(),
		CastleText.Get(),
		CurrencyText.Get()
	};

	for (UTextBlock* Text : TopTexts)
	{
		UHorizontalBoxSlot* TextSlot = TopRow->AddChildToHorizontalBox(Text);
		TextSlot->SetPadding(FMargin(0.0f, 0.0f, 26.0f, 0.0f));
		TextSlot->SetVerticalAlignment(VAlign_Center);
	}

	UBorder* InfoPanel = MakeBattleHUDPanel(WidgetTree, FLinearColor(1.0f, 0.96f, 0.84f, 0.90f), FMargin(16.0f));
	UOverlaySlot* InfoPanelSlot = RootOverlay->AddChildToOverlay(InfoPanel);
	InfoPanelSlot->SetHorizontalAlignment(HAlign_Left);
	InfoPanelSlot->SetVerticalAlignment(VAlign_Top);
	InfoPanelSlot->SetPadding(FMargin(16.0f, 88.0f, 0.0f, 0.0f));

	UVerticalBox* InfoBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	InfoPanel->AddChild(InfoBox);

	SelectedText = MakeBattleHUDText(WidgetTree, FText::GetEmpty(), 16, TTDUIStyle::Ink());
	InventoryText = MakeBattleHUDText(WidgetTree, FText::GetEmpty(), 15, TTDUIStyle::MutedInk());
	InfoBox->AddChildToVerticalBox(SelectedText);
	UVerticalBoxSlot* InventorySlot = InfoBox->AddChildToVerticalBox(InventoryText);
	InventorySlot->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 0.0f));

	UBorder* ActionPanel = MakeBattleHUDPanel(WidgetTree, FLinearColor(0.77f, 0.55f, 0.33f, 0.94f), FMargin(14.0f));
	UOverlaySlot* ActionPanelSlot = RootOverlay->AddChildToOverlay(ActionPanel);
	ActionPanelSlot->SetHorizontalAlignment(HAlign_Right);
	ActionPanelSlot->SetVerticalAlignment(VAlign_Top);
	ActionPanelSlot->SetPadding(FMargin(0.0f, 88.0f, 16.0f, 0.0f));

	UVerticalBox* ActionBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	ActionPanel->AddChild(ActionBox);

	UTextBlock* BuildingTitle = MakeBattleHUDText(WidgetTree, FText::FromString(TEXT("建筑")), 18, TTDUIStyle::Cream(), ETextJustify::Center);
	ActionBox->AddChildToVerticalBox(BuildingTitle);

	BuildingList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UVerticalBoxSlot* BuildingListSlot = ActionBox->AddChildToVerticalBox(BuildingList);
	BuildingListSlot->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 14.0f));

	UTextBlock* ToyBoxTitle = MakeBattleHUDText(WidgetTree, FText::FromString(TEXT("玩具盒")), 18, TTDUIStyle::Cream(), ETextJustify::Center);
	ActionBox->AddChildToVerticalBox(ToyBoxTitle);

	USizeBox* ToyBoxScrollBounds = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	ToyBoxScrollBounds->SetMaxDesiredHeight(390.0f);
	UVerticalBoxSlot* ToyBoxScrollBoundsSlot = ActionBox->AddChildToVerticalBox(ToyBoxScrollBounds);
	ToyBoxScrollBoundsSlot->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 14.0f));

	UScrollBox* ToyBoxScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
	ToyBoxScrollBox->SetScrollBarVisibility(ESlateVisibility::Visible);
	ToyBoxScrollBox->SetScrollbarThickness(FVector2D(6.0f, 6.0f));
	ToyBoxScrollBounds->AddChild(ToyBoxScrollBox);

	ToyBoxList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	ToyBoxScrollBox->AddChild(ToyBoxList);

	FSimpleDelegate CheatDelegate;
	CheatDelegate.BindUObject(this, &UTTDBattleHUDWidget::HandleTestCheatClicked);
	const UTTDDeveloperSettings* Settings = GetDefault<UTTDDeveloperSettings>();
	if (Settings && Settings->bShowBattleTestCheatButton)
	{
		UTTDActionButtonWidget* CheatButton = WidgetTree->ConstructWidget<UTTDActionButtonWidget>(UTTDActionButtonWidget::StaticClass());
		CheatButton->Configure(FText::FromString(TEXT("测试补给")), true, CheatDelegate, ETTDActionButtonVariant::Danger);
		UVerticalBoxSlot* CheatButtonSlot = ActionBox->AddChildToVerticalBox(CheatButton);
		CheatButtonSlot->SetPadding(FMargin(0.0f));
	}

	UBorder* StatusPanel = MakeBattleHUDPanel(WidgetTree, FLinearColor(0.98f, 0.91f, 0.74f, 0.92f), FMargin(18.0f, 10.0f));
	UOverlaySlot* StatusPanelSlot = RootOverlay->AddChildToOverlay(StatusPanel);
	StatusPanelSlot->SetHorizontalAlignment(HAlign_Left);
	StatusPanelSlot->SetVerticalAlignment(VAlign_Bottom);
	StatusPanelSlot->SetPadding(FMargin(16.0f, 0.0f, 0.0f, 16.0f));

	StatusText = MakeBattleHUDText(WidgetTree, FText::FromString(TEXT("准备作战。")), 16, TTDUIStyle::MutedInk());
	StatusPanel->AddChild(StatusText);

	VictoryOverlay = MakeBattleHUDPanel(WidgetTree, FLinearColor(0.188f, 0.149f, 0.114f, 0.72f), FMargin(24.0f));
	VictoryOverlay->SetVisibility(ESlateVisibility::Collapsed);
	UOverlaySlot* VictoryOverlaySlot = RootOverlay->AddChildToOverlay(VictoryOverlay);
	VictoryOverlaySlot->SetHorizontalAlignment(HAlign_Fill);
	VictoryOverlaySlot->SetVerticalAlignment(VAlign_Fill);

	UBorder* VictoryCard = MakeBattleHUDPanel(WidgetTree, TTDUIStyle::Paper(), FMargin(30.0f, 24.0f));
	UVerticalBox* VictoryBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	VictoryCard->AddChild(VictoryBox);
	VictoryBox->AddChildToVerticalBox(MakeBattleHUDText(WidgetTree, FText::FromString(TEXT("胜利")), 38, TTDUIStyle::Ink(), ETextJustify::Center));
	VictoryText = MakeBattleHUDText(WidgetTree, FText::FromString(TEXT("5 秒后返回选关")), 20, TTDUIStyle::MutedInk(), ETextJustify::Center);
	UVerticalBoxSlot* VictoryTextSlot = VictoryBox->AddChildToVerticalBox(VictoryText);
	VictoryTextSlot->SetPadding(FMargin(0.0f, 10.0f, 0.0f, 0.0f));
	VictoryOverlay->AddChild(VictoryCard);
}

void UTTDBattleHUDWidget::BuildActionLists()
{
	UTTDBattleWorldSubsystem* BattleSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UTTDBattleWorldSubsystem>() : nullptr;
	if (!BattleSubsystem || !BuildingList || !ToyBoxList)
	{
		return;
	}

	BuildingList->ClearChildren();
	ToyBoxList->ClearChildren();
	ToyBoxActionWidgets.Reset();
	LastActionListDiagramIds = BattleSubsystem->GetDiagramIds();
	LastActionListDiagramIds.Sort(
		[](const FName& Left, const FName& Right)
		{
			return Left.LexicalLess(Right);
		});

	const TArray<FTTDBuildingDefinition> AvailableBuildingDefinitions = BattleSubsystem->GetAvailableBuildingDefinitions();
	if (AvailableBuildingDefinitions.IsEmpty())
	{
		UTextBlock* EmptyBuildingText = MakeBattleHUDText(WidgetTree, FText::FromString(TEXT("当前携带图纸没有可建造的塔。")), 14, TTDUIStyle::Cream(), ETextJustify::Center);
		BuildingList->AddChildToVerticalBox(EmptyBuildingText);
	}

	for (const FTTDBuildingDefinition& BuildingDefinition : AvailableBuildingDefinitions)
	{
		UHorizontalBox* BuildingRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UVerticalBoxSlot* RowSlot = BuildingList->AddChildToVerticalBox(BuildingRow);
		RowSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

		UTTDActionButtonWidget* Button = WidgetTree->ConstructWidget<UTTDActionButtonWidget>(UTTDActionButtonWidget::StaticClass());
		const FName BuildingId = BuildingDefinition.BuildingId;
		FSimpleDelegate Delegate;
		Delegate.BindWeakLambda(
			this,
			[this, BuildingId]()
			{
				if (ATTDMenuPlayerController* Controller = GetOwningPlayer<ATTDMenuPlayerController>())
				{
					Controller->SelectBattleBuilding(BuildingId);
				}
			});
		Button->Configure(GetBuildingDisplayName(BuildingDefinition), true, Delegate, ETTDActionButtonVariant::Secondary);
		Button->SetHoverText(BuildBuildingHoverText(GetGameInstance(), BuildingDefinition));
		UHorizontalBoxSlot* ButtonSlot = BuildingRow->AddChildToHorizontalBox(Button);
		ButtonSlot->SetVerticalAlignment(VAlign_Center);

		UBorder* CostPanel = MakeBattleHUDPanel(WidgetTree, TTDUIStyle::LightPaper(), FMargin(9.0f, 7.0f));
		CostPanel->SetVisibility(ESlateVisibility::Visible);
		CostPanel->SetToolTipText(BuildBuildingHoverText(GetGameInstance(), BuildingDefinition));
		CostPanel->AddChild(MakeBattleHUDText(WidgetTree, BuildBuildingCostText(GetGameInstance(), BuildingDefinition), 14, TTDUIStyle::Ink(), ETextJustify::Center));
		UHorizontalBoxSlot* CostSlot = BuildingRow->AddChildToHorizontalBox(CostPanel);
		CostSlot->SetPadding(FMargin(8.0f, 0.0f, 0.0f, 0.0f));
		CostSlot->SetVerticalAlignment(VAlign_Center);
	}

	UTextBlock* ShopTitle = MakeBattleHUDText(WidgetTree, FText::FromString(TEXT("固定商品商店")), 16, TTDUIStyle::Cream(), ETextJustify::Center);
	ToyBoxList->AddChildToVerticalBox(ShopTitle);
	const TArray<FTTDNameStack> ToyBoxStacks = BattleSubsystem->GetToyBoxStacks();
	for (const FTTDToyBoxRewardDefinition& ToyBoxDefinition : BattleSubsystem->GetToyBoxDefinitions())
	{
		const FName ToyBoxId = ToyBoxDefinition.ToyBoxId;
		const int32 OwnedCount = FindBattleHUDStackCount(ToyBoxStacks, ToyBoxId);

		UBorder* CardPanel = MakeBattleHUDPanel(WidgetTree, TTDUIStyle::LightPaper(), FMargin(10.0f));
		UVerticalBoxSlot* CardSlot = ToyBoxList->AddChildToVerticalBox(CardPanel);
		CardSlot->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 10.0f));

		UVerticalBox* CardBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		CardPanel->AddChild(CardBox);

		CardBox->AddChildToVerticalBox(MakeBattleHUDText(WidgetTree, TTDUIDisplayNames::ToyBoxName(GetGameInstance(), ToyBoxId, ToyBoxDefinition.DisplayName), 17, TTDUIStyle::Ink(), ETextJustify::Center));
		UTextBlock* ToyBoxSummaryText = MakeBattleHUDText(
			WidgetTree,
			FText::Format(FText::FromString(TEXT("价格 {0}  库存 {1}")), FText::AsNumber(ToyBoxDefinition.PurchaseCost), FText::AsNumber(OwnedCount)),
			14,
			TTDUIStyle::MutedInk(),
			ETextJustify::Center);
		CardBox->AddChildToVerticalBox(ToyBoxSummaryText);

		UTTDActionButtonWidget* BuyButton = WidgetTree->ConstructWidget<UTTDActionButtonWidget>(UTTDActionButtonWidget::StaticClass());
		FSimpleDelegate BuyDelegate;
		BuyDelegate.BindWeakLambda(
			this,
			[this, ToyBoxId]()
			{
				FText FailureReason;
				if (UTTDBattleWorldSubsystem* BattleSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UTTDBattleWorldSubsystem>() : nullptr)
				{
					if (BattleSubsystem->BuyToyBox(ToyBoxId, FailureReason))
					{
						SetStatusMessage(FText::Format(FText::FromString(TEXT("已购买 {0}。")), TTDUIDisplayNames::ToyBoxName(GetGameInstance(), ToyBoxId)));
					}
					else
					{
						SetStatusMessage(FailureReason);
					}
				}
			});
		BuyButton->Configure(FText::FromString(TEXT("购买")), BattleSubsystem->GetCurrency() >= ToyBoxDefinition.PurchaseCost, BuyDelegate, ETTDActionButtonVariant::Primary);
		UVerticalBoxSlot* BuySlot = CardBox->AddChildToVerticalBox(BuyButton);
		BuySlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

		UTTDActionButtonWidget* OpenButton = WidgetTree->ConstructWidget<UTTDActionButtonWidget>(UTTDActionButtonWidget::StaticClass());
		FSimpleDelegate OpenDelegate;
		OpenDelegate.BindWeakLambda(
			this,
			[this, ToyBoxId]()
			{
				TArray<FTTDNameStack> Rewards;
				FText FailureReason;
				if (UTTDBattleWorldSubsystem* BattleSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UTTDBattleWorldSubsystem>() : nullptr)
				{
					if (BattleSubsystem->OpenToyBox(ToyBoxId, Rewards, FailureReason))
					{
						SetStatusMessage(FText::Format(
							FText::FromString(TEXT("已打开 {0}：{1}")),
							TTDUIDisplayNames::ToyBoxName(GetGameInstance(), ToyBoxId),
							FText::FromString(TTDUIDisplayNames::JoinPartStacks(GetGameInstance(), Rewards))));
					}
					else
					{
						SetStatusMessage(FailureReason);
					}
				}
			});
		OpenButton->Configure(FText::FromString(TEXT("打开")), OwnedCount > 0, OpenDelegate, ETTDActionButtonVariant::Secondary);
		CardBox->AddChildToVerticalBox(OpenButton);

		FTTDToyBoxActionWidgets ActionWidgets;
		ActionWidgets.SummaryText = ToyBoxSummaryText;
		ActionWidgets.BuyButton = BuyButton;
		ActionWidgets.OpenButton = OpenButton;
		ActionWidgets.PurchaseCost = ToyBoxDefinition.PurchaseCost;
		ToyBoxActionWidgets.Add(ToyBoxId, ActionWidgets);
	}
}

void UTTDBattleHUDWidget::RefreshActionListStates()
{
	const UTTDBattleWorldSubsystem* BattleSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UTTDBattleWorldSubsystem>() : nullptr;
	if (!BattleSubsystem)
	{
		return;
	}

	const int32 Currency = BattleSubsystem->GetCurrency();
	const TArray<FTTDNameStack> ToyBoxStacks = BattleSubsystem->GetToyBoxStacks();
	for (TPair<FName, FTTDToyBoxActionWidgets>& Pair : ToyBoxActionWidgets)
	{
		const FName ToyBoxId = Pair.Key;
		FTTDToyBoxActionWidgets& ActionWidgets = Pair.Value;
		const int32 OwnedCount = FindBattleHUDStackCount(ToyBoxStacks, ToyBoxId);

		if (UTextBlock* SummaryText = ActionWidgets.SummaryText.Get())
		{
			SummaryText->SetText(FText::Format(
				FText::FromString(TEXT("价格 {0}  库存 {1}")),
				FText::AsNumber(ActionWidgets.PurchaseCost),
				FText::AsNumber(OwnedCount)));
		}

		if (UTTDActionButtonWidget* BuyButton = ActionWidgets.BuyButton.Get())
		{
			BuyButton->SetLabelAndEnabled(FText::FromString(TEXT("购买")), Currency >= ActionWidgets.PurchaseCost);
		}

		if (UTTDActionButtonWidget* OpenButton = ActionWidgets.OpenButton.Get())
		{
			OpenButton->SetLabelAndEnabled(FText::FromString(TEXT("打开")), OwnedCount > 0);
		}
	}
}

void UTTDBattleHUDWidget::RegisterMessageListeners()
{
	UnregisterMessageListeners();

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UGameplayMessageSubsystem* MessageSubsystem = GameInstance->GetSubsystem<UGameplayMessageSubsystem>())
		{
			StateChangedListenerHandle = MessageSubsystem->RegisterListener<FTTDBattleStateChangedMessage>(
				TAG_TTD_Message_Battle_State_Changed,
				this,
				&UTTDBattleHUDWidget::HandleBattleStateChanged);
			PhaseChangedListenerHandle = MessageSubsystem->RegisterListener<FTTDBattlePhaseChangedMessage>(
				TAG_TTD_Message_Battle_Phase_Changed,
				this,
				&UTTDBattleHUDWidget::HandleBattlePhaseChanged);
			ProgressChangedListenerHandle = MessageSubsystem->RegisterListener<FTTDBattleProgressChangedMessage>(
				TAG_TTD_Message_Battle_Progress_Changed,
				this,
				&UTTDBattleHUDWidget::HandleBattleProgressChanged);
			CurrencyChangedListenerHandle = MessageSubsystem->RegisterListener<FTTDBattleCurrencyChangedMessage>(
				TAG_TTD_Message_Battle_Currency_Changed,
				this,
				&UTTDBattleHUDWidget::HandleBattleCurrencyChanged);
			InventoryChangedListenerHandle = MessageSubsystem->RegisterListener<FTTDBattleInventoryChangedMessage>(
				TAG_TTD_Message_Battle_Inventory_Changed,
				this,
				&UTTDBattleHUDWidget::HandleBattleInventoryChanged);
			CastleHealthChangedListenerHandle = MessageSubsystem->RegisterListener<FTTDBattleCastleHealthChangedMessage>(
				TAG_TTD_Message_Battle_CastleHealth_Changed,
				this,
				&UTTDBattleHUDWidget::HandleBattleCastleHealthChanged);
		}
	}
}

void UTTDBattleHUDWidget::UnregisterMessageListeners()
{
	StateChangedListenerHandle.Unregister();
	PhaseChangedListenerHandle.Unregister();
	ProgressChangedListenerHandle.Unregister();
	CurrencyChangedListenerHandle.Unregister();
	InventoryChangedListenerHandle.Unregister();
	CastleHealthChangedListenerHandle.Unregister();
}

void UTTDBattleHUDWidget::RefreshFromBattleSubsystem()
{
	const UTTDBattleWorldSubsystem* BattleSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UTTDBattleWorldSubsystem>() : nullptr;
	if (!BattleSubsystem)
	{
		return;
	}

	RefreshStateText(BattleSubsystem->GetBattleState());
	RefreshPhaseAndWaveText(*BattleSubsystem, true);
	RefreshProgressText(BattleSubsystem->GetProgress(), BattleSubsystem->GetRemainingEnemyWeight(), BattleSubsystem->GetTotalEnemyWeight());
	RefreshCurrencyText(BattleSubsystem->GetCurrency());
	RefreshCastleHealthText(BattleSubsystem->GetCastleCurrentHealth(), BattleSubsystem->GetCastleMaxHealth());
	InventoryText->SetText(BuildInventoryText());
	RefreshActionListStates();
	RefreshSelectedBuilding();
	RefreshVictoryOverlay(*BattleSubsystem, true);
	TryReturnAfterVictory(*BattleSubsystem);
}

void UTTDBattleHUDWidget::RefreshTimedTexts(const bool bForce)
{
	const UTTDBattleWorldSubsystem* BattleSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UTTDBattleWorldSubsystem>() : nullptr;
	if (!BattleSubsystem)
	{
		return;
	}

	RefreshPhaseAndWaveText(*BattleSubsystem, bForce);
	RefreshVictoryOverlay(*BattleSubsystem, bForce);
	TryReturnAfterVictory(*BattleSubsystem);
}

void UTTDBattleHUDWidget::RefreshTimedTextsFromTimer()
{
	RefreshTimedTexts(false);
}

void UTTDBattleHUDWidget::RefreshStateText(const ETTDBattleState State)
{
	if (StateText)
	{
		StateText->SetText(FText::Format(
			FText::FromString(TEXT("状态：{0}")),
			BattleStateToText(State)));
	}
}

void UTTDBattleHUDWidget::RefreshPhaseAndWaveText(const UTTDBattleWorldSubsystem& BattleSubsystem, const bool bForce)
{
	const ETTDBattlePhase Phase = BattleSubsystem.GetBattlePhase();
	const int32 PhaseRemainingSeconds = FMath::CeilToInt(BattleSubsystem.GetPhaseRemainingSeconds());
	const int32 WaveRemainingSeconds = FMath::CeilToInt(BattleSubsystem.GetWaveRemainingSeconds());
	const int32 CurrentWaveNumber = BattleSubsystem.GetCurrentWaveNumber();
	const int32 TotalWaveCount = BattleSubsystem.GetTotalWaveCount();

	if (PhaseText && (bForce || LastPhase != Phase || LastPhaseRemainingSeconds != PhaseRemainingSeconds))
	{
		PhaseText->SetText(FText::Format(
			FText::FromString(TEXT("阶段：{0} {1}s")),
			BattlePhaseToText(Phase),
			FText::AsNumber(PhaseRemainingSeconds)));
		LastPhase = Phase;
		LastPhaseRemainingSeconds = PhaseRemainingSeconds;
	}

	if (WaveText
		&& (bForce
			|| LastWaveRemainingSeconds != WaveRemainingSeconds
			|| LastCurrentWaveNumber != CurrentWaveNumber
			|| LastTotalWaveCount != TotalWaveCount))
	{
		WaveText->SetText(FText::Format(
			FText::FromString(TEXT("波次：{0}/{1}  剩余：{2}s")),
			FText::AsNumber(CurrentWaveNumber),
			FText::AsNumber(TotalWaveCount),
			FText::AsNumber(WaveRemainingSeconds)));
		LastWaveRemainingSeconds = WaveRemainingSeconds;
		LastCurrentWaveNumber = CurrentWaveNumber;
		LastTotalWaveCount = TotalWaveCount;
	}
}

void UTTDBattleHUDWidget::RefreshProgressText(const float Progress, const float RemainingWeight, const float TotalWeight)
{
	if (ProgressText)
	{
		ProgressText->SetText(FText::Format(
			FText::FromString(TEXT("进度：{0}%  剩余权重：{1}/{2}")),
			FText::AsNumber(FMath::RoundToInt(Progress * 100.0f)),
			FText::AsNumber(RemainingWeight),
			FText::AsNumber(TotalWeight)));
	}
}

void UTTDBattleHUDWidget::RefreshCurrencyText(const int32 Currency)
{
	if (CurrencyText)
	{
		CurrencyText->SetText(FText::Format(FText::FromString(TEXT("货币：{0}")), FText::AsNumber(Currency)));
	}
}

void UTTDBattleHUDWidget::RefreshCastleHealthText(const float CurrentHealth, const float MaxHealth)
{
	if (CastleText)
	{
		CastleText->SetText(FText::Format(
			FText::FromString(TEXT("城堡：{0}/{1}")),
			FText::AsNumber(FMath::RoundToInt(CurrentHealth)),
			FText::AsNumber(FMath::RoundToInt(MaxHealth))));
	}
}

void UTTDBattleHUDWidget::RefreshVictoryOverlay(const UTTDBattleWorldSubsystem& BattleSubsystem, const bool bForce)
{
	const bool bShowVictoryOverlay = BattleSubsystem.GetBattleState() == ETTDBattleState::Victory
		&& BattleSubsystem.GetBattlePhase() == ETTDBattlePhase::VictoryDelay;
	if (VictoryOverlay && (bForce || bLastVictoryOverlayVisible != bShowVictoryOverlay))
	{
		VictoryOverlay->SetVisibility(bShowVictoryOverlay ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		bLastVictoryOverlayVisible = bShowVictoryOverlay;
	}

	if (VictoryText && bShowVictoryOverlay)
	{
		const int32 VictoryRemainingSeconds = FMath::CeilToInt(BattleSubsystem.GetPhaseRemainingSeconds());
		if (bForce || LastVictoryRemainingSeconds != VictoryRemainingSeconds)
		{
			VictoryText->SetText(FText::Format(
				FText::FromString(TEXT("{0} 秒后返回选关")),
				FText::AsNumber(VictoryRemainingSeconds)));
			LastVictoryRemainingSeconds = VictoryRemainingSeconds;
		}
	}
	else
	{
		LastVictoryRemainingSeconds = INDEX_NONE;
	}
}

void UTTDBattleHUDWidget::TryReturnAfterVictory(const UTTDBattleWorldSubsystem& BattleSubsystem)
{
	if (BattleSubsystem.IsPostBattleReturnReady() && !bReturnedAfterVictory)
	{
		bReturnedAfterVictory = true;
		if (ATTDMenuPlayerController* Controller = GetOwningPlayer<ATTDMenuPlayerController>())
		{
			Controller->ShowBattleLevelSelect();
		}
	}
}

void UTTDBattleHUDWidget::HandleBattleStateChanged(FGameplayTag Channel, const FTTDBattleStateChangedMessage& Message)
{
	RefreshStateText(Message.State);
	RefreshTimedTexts(true);
}

void UTTDBattleHUDWidget::HandleBattlePhaseChanged(FGameplayTag Channel, const FTTDBattlePhaseChangedMessage& Message)
{
	RefreshTimedTexts(true);
}

void UTTDBattleHUDWidget::HandleBattleProgressChanged(FGameplayTag Channel, const FTTDBattleProgressChangedMessage& Message)
{
	RefreshProgressText(Message.Progress, Message.RemainingWeight, Message.TotalWeight);
}

void UTTDBattleHUDWidget::HandleBattleCurrencyChanged(FGameplayTag Channel, const FTTDBattleCurrencyChangedMessage& Message)
{
	RefreshCurrencyText(Message.Currency);
	RefreshActionListStates();
}

void UTTDBattleHUDWidget::HandleBattleInventoryChanged(FGameplayTag Channel, const FTTDBattleInventoryChangedMessage& Message)
{
	if (InventoryText)
	{
		InventoryText->SetText(BuildInventoryText(Message.DiagramIds, Message.Parts, Message.ToyBoxes));
	}
	TArray<FName> CurrentDiagramIds = Message.DiagramIds;
	CurrentDiagramIds.Sort(
		[](const FName& Left, const FName& Right)
		{
			return Left.LexicalLess(Right);
		});
	if (CurrentDiagramIds != LastActionListDiagramIds)
	{
		BuildActionLists();
	}
	RefreshActionListStates();
}

void UTTDBattleHUDWidget::HandleBattleCastleHealthChanged(FGameplayTag Channel, const FTTDBattleCastleHealthChangedMessage& Message)
{
	RefreshCastleHealthText(Message.CurrentHealth, Message.MaxHealth);
}

FText UTTDBattleHUDWidget::BuildInventoryText() const
{
	const UTTDBattleWorldSubsystem* BattleSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UTTDBattleWorldSubsystem>() : nullptr;
	if (!BattleSubsystem)
	{
		return FText::FromString(TEXT("背包不可用。"));
	}

	const TArray<FName> DiagramIds = BattleSubsystem->GetDiagramIds();
	return BuildInventoryText(DiagramIds, BattleSubsystem->GetPartStacks(), BattleSubsystem->GetToyBoxStacks());
}

FText UTTDBattleHUDWidget::BuildInventoryText(const TArray<FName>& DiagramIds, const TArray<FTTDNameStack>& Parts, const TArray<FTTDNameStack>& ToyBoxes) const
{
	return FText::FromString(FString::Printf(
		TEXT("图纸：%s\n零件：%s\n玩具盒：%s"),
		*TTDUIDisplayNames::JoinDiagramIds(GetGameInstance(), DiagramIds),
		*TTDUIDisplayNames::JoinPartStacks(GetGameInstance(), Parts),
		*TTDUIDisplayNames::JoinToyBoxStacks(GetGameInstance(), ToyBoxes)));
}

void UTTDBattleHUDWidget::HandleTestCheatClicked()
{
	FText Summary;
	if (UTTDBattleWorldSubsystem* BattleSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UTTDBattleWorldSubsystem>() : nullptr)
	{
		BattleSubsystem->ApplyTestCheatSupplies(Summary);
	}
	else
	{
		Summary = FText::FromString(TEXT("战斗系统不可用。"));
	}

	SetStatusMessage(Summary);
	RefreshFromBattleSubsystem();
}
