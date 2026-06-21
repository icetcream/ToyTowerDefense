#include "UI/TTDStartMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "TTDMenuPlayerController.h"
#include "UI/TTDActionButtonWidget.h"
#include "UI/TTDUIStyle.h"

void UTTDStartMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	UBorder* Root = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	TTDUIStyle::ApplyPanel(Root, TTDUIStyle::Backdrop(), FMargin(88.0f, 64.0f));

	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Root->AddChild(Box);
	WidgetTree->RootWidget = Root;

	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Title->SetText(FText::FromString(TEXT("玩具塔防")));
	TTDUIStyle::ApplyText(Title, 64, TTDUIStyle::Ink(), ETextJustify::Center, false);

	UVerticalBoxSlot* TitleSlot = Box->AddChildToVerticalBox(Title);
	TitleSlot->SetHorizontalAlignment(HAlign_Center);
	TitleSlot->SetPadding(FMargin(0.0f, 116.0f, 0.0f, 18.0f));

	UTextBlock* Subtitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Subtitle->SetText(FText::FromString(TEXT("组装、守护、打开下一只玩具盒")));
	TTDUIStyle::ApplyText(Subtitle, 26, TTDUIStyle::MutedInk(), ETextJustify::Center, false);

	UVerticalBoxSlot* SubtitleSlot = Box->AddChildToVerticalBox(Subtitle);
	SubtitleSlot->SetHorizontalAlignment(HAlign_Center);
	SubtitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 54.0f));

	UTTDActionButtonWidget* StartButton = WidgetTree->ConstructWidget<UTTDActionButtonWidget>(UTTDActionButtonWidget::StaticClass());
	FSimpleDelegate StartDelegate;
	StartDelegate.BindUObject(this, &UTTDStartMenuWidget::HandleStartClicked);
	StartButton->Configure(FText::FromString(TEXT("开始游戏")), true, StartDelegate, ETTDActionButtonVariant::Primary);

	UVerticalBoxSlot* ButtonSlot = Box->AddChildToVerticalBox(StartButton);
	ButtonSlot->SetHorizontalAlignment(HAlign_Center);
}

void UTTDStartMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	TTDUIStyle::ResetFadeIn(this, EntranceElapsed);
}

void UTTDStartMenuWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	TTDUIStyle::TickFadeIn(this, EntranceElapsed, InDeltaTime);
}

void UTTDStartMenuWidget::HandleStartClicked()
{
	if (ATTDMenuPlayerController* Controller = GetOwningPlayer<ATTDMenuPlayerController>())
	{
		Controller->ShowMainScreen();
	}
}
