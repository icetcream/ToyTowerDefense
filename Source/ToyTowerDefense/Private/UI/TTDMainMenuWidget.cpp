#include "UI/TTDMainMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "TTDMenuPlayerController.h"
#include "UI/TTDActionButtonWidget.h"
#include "UI/TTDUIStyle.h"

namespace
{
	UTTDActionButtonWidget* MakeMenuButton(UWidgetTree* WidgetTree, UObject* Owner, const FText& Label, void (UTTDMainMenuWidget::*Handler)())
	{
		UTTDActionButtonWidget* Button = WidgetTree->ConstructWidget<UTTDActionButtonWidget>(UTTDActionButtonWidget::StaticClass());
		FSimpleDelegate Delegate;
		Delegate.BindUObject(CastChecked<UTTDMainMenuWidget>(Owner), Handler);
		Button->Configure(Label, true, Delegate, ETTDActionButtonVariant::MenuCard);
		return Button;
	}
}

void UTTDMainMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	UBorder* Root = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	TTDUIStyle::ApplyPanel(Root, TTDUIStyle::Backdrop(), FMargin(54.0f, 48.0f));

	UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Root->AddChild(RootBox);
	WidgetTree->RootWidget = Root;

	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Title->SetText(FText::FromString(TEXT("作战桌面")));
	TTDUIStyle::ApplyText(Title, 46, TTDUIStyle::Ink(), ETextJustify::Center, false);

	UVerticalBoxSlot* TitleSlot = RootBox->AddChildToVerticalBox(Title);
	TitleSlot->SetHorizontalAlignment(HAlign_Center);
	TitleSlot->SetPadding(FMargin(0.0f, 28.0f, 0.0f, 82.0f));

	UHorizontalBox* ButtonRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UVerticalBoxSlot* ButtonRowSlot = RootBox->AddChildToVerticalBox(ButtonRow);
	ButtonRowSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	const TArray<UTTDActionButtonWidget*> Buttons = {
		MakeMenuButton(WidgetTree, this, FText::FromString(TEXT("仓库")), &UTTDMainMenuWidget::HandleWarehouseClicked),
		MakeMenuButton(WidgetTree, this, FText::FromString(TEXT("关卡挑战")), &UTTDMainMenuWidget::HandleLevelChallengeClicked),
		MakeMenuButton(WidgetTree, this, FText::FromString(TEXT("研究所")), &UTTDMainMenuWidget::HandleResearchLabClicked)
	};

	for (UTTDActionButtonWidget* Button : Buttons)
	{
		UHorizontalBoxSlot* ButtonSlot = ButtonRow->AddChildToHorizontalBox(Button);
		ButtonSlot->SetPadding(FMargin(20.0f));
		ButtonSlot->SetHorizontalAlignment(HAlign_Center);
		ButtonSlot->SetVerticalAlignment(VAlign_Center);
		ButtonSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	UBorder* StatusPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	TTDUIStyle::ApplyPanel(StatusPanel, TTDUIStyle::Paper(), FMargin(24.0f, 12.0f));

	StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	StatusText->SetText(FText::FromString(TEXT("选择一个工坊入口继续。")));
	TTDUIStyle::ApplyText(StatusText, 18, TTDUIStyle::MutedInk(), ETextJustify::Center, true);
	StatusPanel->AddChild(StatusText);

	UVerticalBoxSlot* StatusSlot = RootBox->AddChildToVerticalBox(StatusPanel);
	StatusSlot->SetPadding(FMargin(0.0f, 54.0f, 0.0f, 0.0f));
	StatusSlot->SetHorizontalAlignment(HAlign_Fill);
}

void UTTDMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	TTDUIStyle::ResetFadeIn(this, EntranceElapsed);
}

void UTTDMainMenuWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	TTDUIStyle::TickFadeIn(this, EntranceElapsed, InDeltaTime);
}

void UTTDMainMenuWidget::HandleWarehouseClicked()
{
	if (ATTDMenuPlayerController* Controller = GetOwningPlayer<ATTDMenuPlayerController>())
	{
		Controller->ShowWarehouse();
	}
}

void UTTDMainMenuWidget::HandleLevelChallengeClicked()
{
	if (ATTDMenuPlayerController* Controller = GetOwningPlayer<ATTDMenuPlayerController>())
	{
		Controller->ShowBattleLevelSelect();
		return;
	}

	if (StatusText)
	{
		StatusText->SetText(FText::FromString(TEXT("关卡挑战还没有配置可用关卡。")));
	}
}

void UTTDMainMenuWidget::HandleResearchLabClicked()
{
	if (ATTDMenuPlayerController* Controller = GetOwningPlayer<ATTDMenuPlayerController>())
	{
		Controller->ShowResearchLab();
	}
}
