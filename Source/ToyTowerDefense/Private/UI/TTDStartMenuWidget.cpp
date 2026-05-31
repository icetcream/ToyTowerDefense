#include "UI/TTDStartMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "TTDMenuPlayerController.h"
#include "UI/TTDActionButtonWidget.h"

void UTTDStartMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	UBorder* Root = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Root->SetBrushColor(FLinearColor(0.03f, 0.05f, 0.07f, 1.0f));
	Root->SetPadding(FMargin(80.0f));

	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Root->AddChild(Box);
	WidgetTree->RootWidget = Root;

	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Title->SetText(FText::FromString(TEXT("玩具塔防")));
	Title->SetJustification(ETextJustify::Center);
	Title->SetColorAndOpacity(FSlateColor(FLinearColor(0.98f, 0.82f, 0.25f, 1.0f)));
	FSlateFontInfo TitleFont = Title->GetFont();
	TitleFont.Size = 56;
	Title->SetFont(TitleFont);

	UVerticalBoxSlot* TitleSlot = Box->AddChildToVerticalBox(Title);
	TitleSlot->SetHorizontalAlignment(HAlign_Center);
	TitleSlot->SetPadding(FMargin(0.0f, 120.0f, 0.0f, 40.0f));

	UTextBlock* Subtitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Subtitle->SetText(FText::FromString(TEXT("开始界面")));
	Subtitle->SetJustification(ETextJustify::Center);
	Subtitle->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	FSlateFontInfo SubtitleFont = Subtitle->GetFont();
	SubtitleFont.Size = 26;
	Subtitle->SetFont(SubtitleFont);

	UVerticalBoxSlot* SubtitleSlot = Box->AddChildToVerticalBox(Subtitle);
	SubtitleSlot->SetHorizontalAlignment(HAlign_Center);
	SubtitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 40.0f));

	UTTDActionButtonWidget* StartButton = WidgetTree->ConstructWidget<UTTDActionButtonWidget>(UTTDActionButtonWidget::StaticClass());
	FSimpleDelegate StartDelegate;
	StartDelegate.BindUObject(this, &UTTDStartMenuWidget::HandleStartClicked);
	StartButton->Configure(FText::FromString(TEXT("开始游戏")), true, StartDelegate);

	UVerticalBoxSlot* ButtonSlot = Box->AddChildToVerticalBox(StartButton);
	ButtonSlot->SetHorizontalAlignment(HAlign_Center);
}

void UTTDStartMenuWidget::HandleStartClicked()
{
	if (ATTDMenuPlayerController* Controller = GetOwningPlayer<ATTDMenuPlayerController>())
	{
		Controller->ShowMainScreen();
	}
}
