#include "UI/TTDActionButtonWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

void UTTDActionButtonWidget::Configure(const FText& InLabel, const bool bInEnabled, const FSimpleDelegate& InClickHandler)
{
	ButtonLabel = InLabel;
	bButtonEnabled = bInEnabled;
	ClickHandler = InClickHandler;
	ApplyConfiguration();
}

void UTTDActionButtonWidget::SetLabelAndEnabled(const FText& InLabel, const bool bInEnabled)
{
	ButtonLabel = InLabel;
	bButtonEnabled = bInEnabled;
	ApplyConfiguration();
}

void UTTDActionButtonWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Label->SetJustification(ETextJustify::Center);
	Label->SetAutoWrapText(true);
	Label->SetColorAndOpacity(FSlateColor(FLinearColor::Black));

	Button->AddChild(Label);
	Button->OnClicked.AddDynamic(this, &UTTDActionButtonWidget::HandleClicked);
	WidgetTree->RootWidget = Button;

	ApplyConfiguration();
}

void UTTDActionButtonWidget::HandleClicked()
{
	if (ClickHandler.IsBound())
	{
		ClickHandler.Execute();
	}
}

void UTTDActionButtonWidget::ApplyConfiguration()
{
	if (Button)
	{
		Button->SetIsEnabled(bButtonEnabled);
	}

	if (Label)
	{
		Label->SetText(ButtonLabel);
	}
}
