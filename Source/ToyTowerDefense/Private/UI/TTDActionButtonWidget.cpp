#include "UI/TTDActionButtonWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "UI/TTDUIStyle.h"

void UTTDActionButtonWidget::Configure(const FText& InLabel, const bool bInEnabled, const FSimpleDelegate& InClickHandler, const ETTDActionButtonVariant InVariant)
{
	ButtonLabel = InLabel;
	bButtonEnabled = bInEnabled;
	ClickHandler = InClickHandler;
	VisualVariant = InVariant;
	ApplyConfiguration();
}

void UTTDActionButtonWidget::SetLabelAndEnabled(const FText& InLabel, const bool bInEnabled)
{
	if (ButtonLabel.EqualTo(InLabel) && bButtonEnabled == bInEnabled)
	{
		return;
	}

	ButtonLabel = InLabel;
	bButtonEnabled = bInEnabled;
	ApplyConfiguration();
}

void UTTDActionButtonWidget::SetVisualVariant(const ETTDActionButtonVariant InVariant)
{
	VisualVariant = InVariant;
	ApplyConfiguration();
}

void UTTDActionButtonWidget::SetPulseActive(const bool bInPulseActive)
{
	bPulseActive = bInPulseActive;
	if (!bPulseActive)
	{
		PulseSeconds = 0.0f;
	}
}

void UTTDActionButtonWidget::SetHoverText(const FText& InHoverText)
{
	HoverText = InHoverText;
	ApplyConfiguration();
}

void UTTDActionButtonWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Label->SetJustification(ETextJustify::Center);
	Label->SetAutoWrapText(true);
	Label->SetColorAndOpacity(FSlateColor(TTDUIStyle::Ink()));

	Button->AddChild(Label);
	Button->OnClicked.AddDynamic(this, &UTTDActionButtonWidget::HandleClicked);
	Button->OnHovered.AddDynamic(this, &UTTDActionButtonWidget::HandleHovered);
	Button->OnUnhovered.AddDynamic(this, &UTTDActionButtonWidget::HandleUnhovered);
	Button->OnPressed.AddDynamic(this, &UTTDActionButtonWidget::HandlePressed);
	Button->OnReleased.AddDynamic(this, &UTTDActionButtonWidget::HandleReleased);
	SizeBox->AddChild(Button);
	WidgetTree->RootWidget = SizeBox;

	SetRenderTransformPivot(FVector2D(0.5f, 0.5f));

	ApplyConfiguration();
}

void UTTDActionButtonWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bPulseActive && !bHovered && !bPressed && FMath::IsNearlyEqual(CurrentScale, 1.0f, 0.001f))
	{
		return;
	}

	PulseSeconds += InDeltaTime;
	const float PulseScale = bPulseActive ? FMath::Sin(PulseSeconds * 6.0f) * 0.018f : 0.0f;
	const float TargetScale = (bPressed ? 0.975f : (bHovered ? 1.025f : 1.0f)) + PulseScale;
	CurrentScale = FMath::FInterpTo(CurrentScale, TargetScale, InDeltaTime, 12.0f);
	SetRenderScale(FVector2D(CurrentScale, CurrentScale));
}

void UTTDActionButtonWidget::HandleClicked()
{
	if (ClickHandler.IsBound())
	{
		ClickHandler.Execute();
	}
}

void UTTDActionButtonWidget::HandleHovered()
{
	bHovered = true;
}

void UTTDActionButtonWidget::HandleUnhovered()
{
	bHovered = false;
	bPressed = false;
}

void UTTDActionButtonWidget::HandlePressed()
{
	bPressed = true;
}

void UTTDActionButtonWidget::HandleReleased()
{
	bPressed = false;
}

void UTTDActionButtonWidget::ApplyConfiguration()
{
	if (Button)
	{
		Button->SetIsEnabled(bButtonEnabled);
		Button->SetStyle(TTDUIStyle::MakeButtonStyle(VisualVariant, bButtonEnabled));
		Button->SetToolTipText(HoverText);
	}

	if (Label)
	{
		Label->SetText(ButtonLabel);
		int32 FontSize = 18;
		switch (VisualVariant)
		{
		case ETTDActionButtonVariant::Primary:
			FontSize = 26;
			break;
		case ETTDActionButtonVariant::MenuCard:
			FontSize = 34;
			break;
		case ETTDActionButtonVariant::QueueSlot:
			FontSize = 15;
			break;
		case ETTDActionButtonVariant::LevelNode:
		case ETTDActionButtonVariant::LevelNodeSelected:
		case ETTDActionButtonVariant::LevelNodeLocked:
			FontSize = 22;
			break;
		case ETTDActionButtonVariant::LibraryCard:
		case ETTDActionButtonVariant::LibraryCardSelected:
			FontSize = 15;
			break;
		case ETTDActionButtonVariant::EmptySlot:
		case ETTDActionButtonVariant::FilledSlot:
			FontSize = 14;
			break;
		case ETTDActionButtonVariant::Ghost:
			FontSize = 16;
			break;
		case ETTDActionButtonVariant::Danger:
		case ETTDActionButtonVariant::Secondary:
		default:
			FontSize = 18;
			break;
		}

		TTDUIStyle::ApplyText(Label, FontSize, bButtonEnabled ? TTDUIStyle::Ink() : TTDUIStyle::MutedInk(), ETextJustify::Center, true);
	}

	ApplyVariantSizing();
}

void UTTDActionButtonWidget::ApplyVariantSizing()
{
	if (!SizeBox)
	{
		return;
	}

	float MinWidth = 132.0f;
	float MinHeight = 48.0f;

	switch (VisualVariant)
	{
	case ETTDActionButtonVariant::Primary:
		MinWidth = 240.0f;
		MinHeight = 66.0f;
		break;
	case ETTDActionButtonVariant::MenuCard:
		MinWidth = 240.0f;
		MinHeight = 184.0f;
		break;
	case ETTDActionButtonVariant::QueueSlot:
		MinWidth = 118.0f;
		MinHeight = 86.0f;
		break;
	case ETTDActionButtonVariant::LevelNode:
	case ETTDActionButtonVariant::LevelNodeSelected:
	case ETTDActionButtonVariant::LevelNodeLocked:
		MinWidth = 74.0f;
		MinHeight = 58.0f;
		break;
	case ETTDActionButtonVariant::LibraryCard:
	case ETTDActionButtonVariant::LibraryCardSelected:
		MinWidth = 126.0f;
		MinHeight = 78.0f;
		break;
	case ETTDActionButtonVariant::EmptySlot:
	case ETTDActionButtonVariant::FilledSlot:
		MinWidth = 86.0f;
		MinHeight = 58.0f;
		break;
	case ETTDActionButtonVariant::Ghost:
		MinWidth = 118.0f;
		MinHeight = 38.0f;
		break;
	case ETTDActionButtonVariant::Danger:
	case ETTDActionButtonVariant::Secondary:
	default:
		break;
	}

	SizeBox->SetMinDesiredWidth(MinWidth);
	SizeBox->SetMinDesiredHeight(MinHeight);
}
