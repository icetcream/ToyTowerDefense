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

namespace
{
	UTTDActionButtonWidget* MakeMenuButton(UWidgetTree* WidgetTree, UObject* Owner, const FText& Label, void (UTTDMainMenuWidget::*Handler)())
	{
		UTTDActionButtonWidget* Button = WidgetTree->ConstructWidget<UTTDActionButtonWidget>(UTTDActionButtonWidget::StaticClass());
		FSimpleDelegate Delegate;
		Delegate.BindUObject(CastChecked<UTTDMainMenuWidget>(Owner), Handler);
		Button->Configure(Label, true, Delegate);
		return Button;
	}
}

void UTTDMainMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	UBorder* Root = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Root->SetBrushColor(FLinearColor(0.92f, 0.96f, 1.0f, 1.0f));
	Root->SetPadding(FMargin(60.0f));

	UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Root->AddChild(RootBox);
	WidgetTree->RootWidget = Root;

	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Title->SetText(FText::FromString(TEXT("主界面")));
	Title->SetJustification(ETextJustify::Center);
	Title->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
	FSlateFontInfo TitleFont = Title->GetFont();
	TitleFont.Size = 40;
	Title->SetFont(TitleFont);

	UVerticalBoxSlot* TitleSlot = RootBox->AddChildToVerticalBox(Title);
	TitleSlot->SetHorizontalAlignment(HAlign_Center);
	TitleSlot->SetPadding(FMargin(0.0f, 40.0f, 0.0f, 120.0f));

	UHorizontalBox* ButtonRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	RootBox->AddChildToVerticalBox(ButtonRow);

	const TArray<UTTDActionButtonWidget*> Buttons = {
		MakeMenuButton(WidgetTree, this, FText::FromString(TEXT("仓库")), &UTTDMainMenuWidget::HandleWarehouseClicked),
		MakeMenuButton(WidgetTree, this, FText::FromString(TEXT("关卡挑战")), &UTTDMainMenuWidget::HandleLevelChallengeClicked),
		MakeMenuButton(WidgetTree, this, FText::FromString(TEXT("研究所")), &UTTDMainMenuWidget::HandleResearchLabClicked)
	};

	for (UTTDActionButtonWidget* Button : Buttons)
	{
		UHorizontalBoxSlot* ButtonSlot = ButtonRow->AddChildToHorizontalBox(Button);
		ButtonSlot->SetPadding(FMargin(24.0f));
		ButtonSlot->SetHorizontalAlignment(HAlign_Center);
		ButtonSlot->SetVerticalAlignment(VAlign_Center);
		ButtonSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	StatusText->SetText(FText::FromString(TEXT("点击对应按钮进入对应界面")));
	StatusText->SetJustification(ETextJustify::Center);
	StatusText->SetColorAndOpacity(FSlateColor(FLinearColor::Black));

	UVerticalBoxSlot* StatusSlot = RootBox->AddChildToVerticalBox(StatusText);
	StatusSlot->SetPadding(FMargin(0.0f, 80.0f, 0.0f, 0.0f));
	StatusSlot->SetHorizontalAlignment(HAlign_Center);
}

void UTTDMainMenuWidget::HandleWarehouseClicked()
{
	if (StatusText)
	{
		StatusText->SetText(FText::FromString(TEXT("仓库界面待接入资源背包。")));
	}
}

void UTTDMainMenuWidget::HandleLevelChallengeClicked()
{
	if (ATTDMenuPlayerController* Controller = GetOwningPlayer<ATTDMenuPlayerController>())
	{
		if (Controller->ShowBattleLevel())
		{
			return;
		}

		if (StatusText)
		{
			StatusText->SetText(Controller->GetLastBattleFailureReason());
			return;
		}
	}

	if (StatusText)
	{
		StatusText->SetText(FText::FromString(TEXT("Battle level is not configured yet.")));
	}
}

void UTTDMainMenuWidget::HandleResearchLabClicked()
{
	if (ATTDMenuPlayerController* Controller = GetOwningPlayer<ATTDMenuPlayerController>())
	{
		Controller->ShowResearchLab();
	}
}
