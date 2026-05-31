#include "UI/TTDBattleHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "TTDBattleWorldSubsystem.h"
#include "TTDMenuPlayerController.h"
#include "UI/TTDActionButtonWidget.h"

namespace
{
	UTextBlock* MakeText(UWidgetTree* WidgetTree, const int32 FontSize)
	{
		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Text->SetAutoWrapText(true);
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = FontSize;
		Text->SetFont(Font);
		return Text;
	}

	FString JoinStacks(const TArray<FTTDNameStack>& Stacks)
	{
		TArray<FString> Parts;
		for (const FTTDNameStack& Stack : Stacks)
		{
			if (!Stack.Id.IsNone() && Stack.Count > 0)
			{
				Parts.Add(FString::Printf(TEXT("%s x%d"), *Stack.Id.ToString(), Stack.Count));
			}
		}
		return Parts.IsEmpty() ? FString(TEXT("None")) : FString::Join(Parts, TEXT(", "));
	}
}

void UTTDBattleHUDWidget::SetStatusMessage(const FText& Message)
{
	if (StatusText)
	{
		StatusText->SetText(Message);
	}
}

void UTTDBattleHUDWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	BuildLayout();
	BuildActionLists();
	Refresh();
}

void UTTDBattleHUDWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Refresh();
}

void UTTDBattleHUDWidget::BuildLayout()
{
	UBorder* Root = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Root->SetBrushColor(FLinearColor(0.06f, 0.07f, 0.08f, 0.82f));
	Root->SetPadding(FMargin(24.0f));
	WidgetTree->RootWidget = Root;

	UHorizontalBox* RootRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Root->AddChild(RootRow);

	UVerticalBox* StatusPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UHorizontalBoxSlot* StatusPanelSlot = RootRow->AddChildToHorizontalBox(StatusPanel);
	StatusPanelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	StatusPanelSlot->SetPadding(FMargin(0.0f, 0.0f, 24.0f, 0.0f));

	StateText = MakeText(WidgetTree, 24);
	ProgressText = MakeText(WidgetTree, 18);
	CastleText = MakeText(WidgetTree, 18);
	CurrencyText = MakeText(WidgetTree, 18);
	InventoryText = MakeText(WidgetTree, 16);
	SelectedText = MakeText(WidgetTree, 16);
	StatusText = MakeText(WidgetTree, 16);

	TArray<UTextBlock*> TextBlocks = {
		StateText.Get(),
		ProgressText.Get(),
		CastleText.Get(),
		CurrencyText.Get(),
		InventoryText.Get(),
		SelectedText.Get(),
		StatusText.Get()
	};

	for (UTextBlock* Text : TextBlocks)
	{
		Text->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		UVerticalBoxSlot* TextSlot = StatusPanel->AddChildToVerticalBox(Text);
		TextSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
	}

	UVerticalBox* ActionPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UHorizontalBoxSlot* ActionPanelSlot = RootRow->AddChildToHorizontalBox(ActionPanel);
	ActionPanelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

	UTextBlock* BuildingTitle = MakeText(WidgetTree, 18);
	BuildingTitle->SetText(FText::FromString(TEXT("Buildings")));
	BuildingTitle->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	ActionPanel->AddChildToVerticalBox(BuildingTitle);

	BuildingList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	ActionPanel->AddChildToVerticalBox(BuildingList);

	UTextBlock* ToyBoxTitle = MakeText(WidgetTree, 18);
	ToyBoxTitle->SetText(FText::FromString(TEXT("Toy Boxes")));
	ToyBoxTitle->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	UVerticalBoxSlot* ToyBoxTitleSlot = ActionPanel->AddChildToVerticalBox(ToyBoxTitle);
	ToyBoxTitleSlot->SetPadding(FMargin(0.0f, 20.0f, 0.0f, 0.0f));

	ToyBoxList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	ActionPanel->AddChildToVerticalBox(ToyBoxList);
}

void UTTDBattleHUDWidget::BuildActionLists()
{
	UTTDBattleWorldSubsystem* BattleSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UTTDBattleWorldSubsystem>() : nullptr;
	if (!BattleSubsystem)
	{
		return;
	}

	for (const FTTDBuildingDefinition& BuildingDefinition : BattleSubsystem->GetBuildingDefinitions())
	{
		UTTDActionButtonWidget* Button = WidgetTree->ConstructWidget<UTTDActionButtonWidget>(UTTDActionButtonWidget::StaticClass());
		const FName BuildingId = BuildingDefinition.BuildingId;
		FSimpleDelegate Delegate;
		Delegate.BindLambda(
			[this, BuildingId]()
			{
				if (ATTDMenuPlayerController* Controller = GetOwningPlayer<ATTDMenuPlayerController>())
				{
					Controller->SelectBattleBuilding(BuildingId);
				}
			});
		Button->Configure(FText::FromName(BuildingId), true, Delegate);
		UVerticalBoxSlot* ButtonSlot = BuildingList->AddChildToVerticalBox(Button);
		ButtonSlot->SetPadding(FMargin(0.0f, 6.0f));
	}

	for (const FTTDToyBoxRewardDefinition& ToyBoxDefinition : BattleSubsystem->GetToyBoxDefinitions())
	{
		const FName ToyBoxId = ToyBoxDefinition.ToyBoxId;

		UTTDActionButtonWidget* BuyButton = WidgetTree->ConstructWidget<UTTDActionButtonWidget>(UTTDActionButtonWidget::StaticClass());
		FSimpleDelegate BuyDelegate;
		BuyDelegate.BindLambda(
			[this, ToyBoxId]()
			{
				FText FailureReason;
				if (UTTDBattleWorldSubsystem* BattleSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UTTDBattleWorldSubsystem>() : nullptr)
				{
					if (BattleSubsystem->BuyToyBox(ToyBoxId, FailureReason))
					{
						SetStatusMessage(FText::Format(FText::FromString(TEXT("Bought {0}.")), FText::FromName(ToyBoxId)));
					}
					else
					{
						SetStatusMessage(FailureReason);
					}
				}
			});
		BuyButton->Configure(FText::Format(FText::FromString(TEXT("Buy {0}")), FText::FromName(ToyBoxId)), true, BuyDelegate);
		UVerticalBoxSlot* BuySlot = ToyBoxList->AddChildToVerticalBox(BuyButton);
		BuySlot->SetPadding(FMargin(0.0f, 6.0f));

		UTTDActionButtonWidget* OpenButton = WidgetTree->ConstructWidget<UTTDActionButtonWidget>(UTTDActionButtonWidget::StaticClass());
		FSimpleDelegate OpenDelegate;
		OpenDelegate.BindLambda(
			[this, ToyBoxId]()
			{
				TArray<FTTDNameStack> Rewards;
				FText FailureReason;
				if (UTTDBattleWorldSubsystem* BattleSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UTTDBattleWorldSubsystem>() : nullptr)
				{
					if (BattleSubsystem->OpenToyBox(ToyBoxId, Rewards, FailureReason))
					{
						SetStatusMessage(FText::Format(FText::FromString(TEXT("Opened {0}: {1}")), FText::FromName(ToyBoxId), FText::FromString(JoinStacks(Rewards))));
					}
					else
					{
						SetStatusMessage(FailureReason);
					}
				}
			});
		OpenButton->Configure(FText::Format(FText::FromString(TEXT("Open {0}")), FText::FromName(ToyBoxId)), true, OpenDelegate);
		UVerticalBoxSlot* OpenSlot = ToyBoxList->AddChildToVerticalBox(OpenButton);
		OpenSlot->SetPadding(FMargin(0.0f, 6.0f));
	}
}

void UTTDBattleHUDWidget::Refresh()
{
	const UTTDBattleWorldSubsystem* BattleSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UTTDBattleWorldSubsystem>() : nullptr;
	if (!BattleSubsystem)
	{
		return;
	}

	StateText->SetText(FText::Format(
		FText::FromString(TEXT("State: {0}")),
		StaticEnum<ETTDBattleState>()->GetDisplayNameTextByValue(static_cast<int64>(BattleSubsystem->GetBattleState()))));
	ProgressText->SetText(FText::Format(
		FText::FromString(TEXT("Progress: {0}%  Remaining Weight: {1}/{2}")),
		FText::AsNumber(FMath::RoundToInt(BattleSubsystem->GetProgress() * 100.0f)),
		FText::AsNumber(BattleSubsystem->GetRemainingEnemyWeight()),
		FText::AsNumber(BattleSubsystem->GetTotalEnemyWeight())));
	CastleText->SetText(FText::Format(
		FText::FromString(TEXT("Castle: {0}/{1}")),
		FText::AsNumber(FMath::RoundToInt(BattleSubsystem->GetCastleCurrentHealth())),
		FText::AsNumber(FMath::RoundToInt(BattleSubsystem->GetCastleMaxHealth()))));
	CurrencyText->SetText(FText::Format(FText::FromString(TEXT("Currency: {0}")), FText::AsNumber(BattleSubsystem->GetCurrency())));
	InventoryText->SetText(BuildInventoryText());

	const ATTDMenuPlayerController* Controller = GetOwningPlayer<ATTDMenuPlayerController>();
	const FName SelectedBuildingId = Controller ? Controller->GetSelectedBattleBuildingId() : NAME_None;
	SelectedText->SetText(SelectedBuildingId.IsNone()
		? FText::FromString(TEXT("Selected: None"))
		: FText::Format(FText::FromString(TEXT("Selected: {0}")), FText::FromName(SelectedBuildingId)));
}

FText UTTDBattleHUDWidget::BuildInventoryText() const
{
	const UTTDBattleWorldSubsystem* BattleSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UTTDBattleWorldSubsystem>() : nullptr;
	if (!BattleSubsystem)
	{
		return FText::FromString(TEXT("Inventory unavailable."));
	}

	const TArray<FName> DiagramIds = BattleSubsystem->GetDiagramIds();
	const FString Diagrams = DiagramIds.IsEmpty()
		? FString(TEXT("None"))
		: FString::JoinBy(DiagramIds, TEXT(", "), [](const FName DiagramId) { return DiagramId.ToString(); });

	return FText::FromString(FString::Printf(
		TEXT("Diagrams: %s\nParts: %s\nToy Boxes: %s"),
		*Diagrams,
		*JoinStacks(BattleSubsystem->GetPartStacks()),
		*JoinStacks(BattleSubsystem->GetToyBoxStacks())));
}
