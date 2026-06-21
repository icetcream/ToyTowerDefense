#include "UI/TTDUIStyle.h"

#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "UI/TTDActionButtonWidget.h"

namespace
{
	FLinearColor WithAlpha(const FLinearColor& Color, const float Alpha)
	{
		return FLinearColor(Color.R, Color.G, Color.B, Alpha);
	}

	FLinearColor Mix(const FLinearColor& A, const FLinearColor& B, const float Amount)
	{
		return FMath::Lerp(A, B, Amount);
	}

	FButtonStyle MakeStyle(const FLinearColor& Normal, const FLinearColor& Hovered, const FLinearColor& Pressed, const FMargin& Padding)
	{
		FButtonStyle Style;
		Style.SetNormal(TTDUIStyle::MakeBoxBrush(Normal));
		Style.SetHovered(TTDUIStyle::MakeBoxBrush(Hovered));
		Style.SetPressed(TTDUIStyle::MakeBoxBrush(Pressed));
		Style.SetDisabled(TTDUIStyle::MakeBoxBrush(WithAlpha(Mix(Normal, FLinearColor::Gray, 0.55f), 0.62f)));
		Style.SetNormalPadding(Padding);
		Style.SetPressedPadding(Padding + FMargin(1.0f, 2.0f, 1.0f, 0.0f));
		return Style;
	}
}

namespace TTDUIStyle
{
	FLinearColor Backdrop()
	{
		return FLinearColor(0.83f, 0.50f, 0.26f, 1.0f);
	}

	FLinearColor Paper()
	{
		return FLinearColor(1.0f, 0.953f, 0.831f, 1.0f);
	}

	FLinearColor LightPaper()
	{
		return FLinearColor(1.0f, 0.976f, 0.914f, 1.0f);
	}

	FLinearColor Cardboard()
	{
		return FLinearColor(0.77f, 0.55f, 0.33f, 1.0f);
	}

	FLinearColor Wood()
	{
		return FLinearColor(0.722f, 0.459f, 0.239f, 1.0f);
	}

	FLinearColor DarkWood()
	{
		return FLinearColor(0.439f, 0.251f, 0.122f, 1.0f);
	}

	FLinearColor Ink()
	{
		return FLinearColor(0.188f, 0.149f, 0.114f, 1.0f);
	}

	FLinearColor MutedInk()
	{
		return FLinearColor(0.443f, 0.392f, 0.325f, 1.0f);
	}

	FLinearColor Cream()
	{
		return FLinearColor(1.0f, 0.96f, 0.84f, 1.0f);
	}

	FLinearColor Blue()
	{
		return FLinearColor(0.184f, 0.561f, 0.780f, 1.0f);
	}

	FLinearColor Green()
	{
		return FLinearColor(0.384f, 0.659f, 0.373f, 1.0f);
	}

	FLinearColor Orange()
	{
		return FLinearColor(0.890f, 0.604f, 0.212f, 1.0f);
	}

	FLinearColor Success()
	{
		return FLinearColor(0.35f, 0.66f, 0.45f, 1.0f);
	}

	FLinearColor Warning()
	{
		return FLinearColor(0.94f, 0.68f, 0.23f, 1.0f);
	}

	FLinearColor Danger()
	{
		return FLinearColor(0.75f, 0.24f, 0.21f, 1.0f);
	}

	FLinearColor Line()
	{
		return FLinearColor(0.188f, 0.149f, 0.114f, 0.20f);
	}

	FSlateBrush MakeBoxBrush(const FLinearColor& Color, const float Margin)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::Box;
		Brush.Margin = FMargin(Margin);
		Brush.TintColor = FSlateColor(Color);
		Brush.ImageSize = FVector2D(32.0f, 32.0f);
		return Brush;
	}

	FSlateBrush MakeTransparentBrush()
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::NoDrawType;
		Brush.TintColor = FSlateColor(FLinearColor::Transparent);
		return Brush;
	}

	FButtonStyle MakeButtonStyle(const ETTDActionButtonVariant Variant, const bool bEnabled)
	{
		const FLinearColor DisabledBase = WithAlpha(FLinearColor(0.55f, 0.52f, 0.46f, 1.0f), 0.58f);

		if (!bEnabled)
		{
			return MakeStyle(DisabledBase, DisabledBase, DisabledBase, FMargin(18.0f, 10.0f));
		}

		switch (Variant)
		{
		case ETTDActionButtonVariant::Primary:
			return MakeStyle(Blue(), FLinearColor(0.255f, 0.714f, 0.910f, 1.0f), FLinearColor(0.137f, 0.490f, 0.733f, 1.0f), FMargin(30.0f, 16.0f));
		case ETTDActionButtonVariant::MenuCard:
			return MakeStyle(FLinearColor(0.93f, 0.56f, 0.36f, 1.0f), FLinearColor(0.99f, 0.66f, 0.44f, 1.0f), FLinearColor(0.75f, 0.36f, 0.25f, 1.0f), FMargin(34.0f, 44.0f));
		case ETTDActionButtonVariant::QueueSlot:
			return MakeStyle(FLinearColor(0.58f, 0.74f, 0.78f, 1.0f), FLinearColor(0.66f, 0.83f, 0.87f, 1.0f), FLinearColor(0.42f, 0.58f, 0.63f, 1.0f), FMargin(14.0f, 12.0f));
		case ETTDActionButtonVariant::Danger:
			return MakeStyle(Danger(), FLinearColor(0.88f, 0.32f, 0.27f, 1.0f), FLinearColor(0.55f, 0.13f, 0.13f, 1.0f), FMargin(20.0f, 11.0f));
		case ETTDActionButtonVariant::Ghost:
			return MakeStyle(LightPaper(), FLinearColor(1.0f, 0.925f, 0.667f, 1.0f), Paper(), FMargin(16.0f, 8.0f));
		case ETTDActionButtonVariant::LevelNode:
			return MakeStyle(FLinearColor(1.0f, 0.945f, 0.749f, 1.0f), FLinearColor(1.0f, 0.984f, 0.831f, 1.0f), FLinearColor(0.906f, 0.733f, 0.431f, 1.0f), FMargin(14.0f, 12.0f));
		case ETTDActionButtonVariant::LevelNodeSelected:
			return MakeStyle(FLinearColor(0.518f, 0.812f, 1.0f, 1.0f), FLinearColor(0.620f, 0.878f, 1.0f, 1.0f), FLinearColor(0.282f, 0.631f, 0.851f, 1.0f), FMargin(14.0f, 12.0f));
		case ETTDActionButtonVariant::LevelNodeLocked:
			return MakeStyle(FLinearColor(0.50f, 0.48f, 0.43f, 0.50f), FLinearColor(0.50f, 0.48f, 0.43f, 0.50f), FLinearColor(0.50f, 0.48f, 0.43f, 0.50f), FMargin(14.0f, 12.0f));
		case ETTDActionButtonVariant::LibraryCard:
			return MakeStyle(LightPaper(), FLinearColor(1.0f, 0.953f, 0.765f, 1.0f), Paper(), FMargin(12.0f, 9.0f));
		case ETTDActionButtonVariant::LibraryCardSelected:
			return MakeStyle(FLinearColor(0.875f, 0.945f, 1.0f, 1.0f), FLinearColor(0.925f, 0.973f, 1.0f, 1.0f), FLinearColor(0.769f, 0.890f, 0.984f, 1.0f), FMargin(12.0f, 9.0f));
		case ETTDActionButtonVariant::EmptySlot:
			return MakeStyle(FLinearColor(1.0f, 1.0f, 1.0f, 0.34f), FLinearColor(1.0f, 0.976f, 0.914f, 0.58f), FLinearColor(1.0f, 0.953f, 0.831f, 0.62f), FMargin(10.0f, 8.0f));
		case ETTDActionButtonVariant::FilledSlot:
			return MakeStyle(FLinearColor(0.867f, 0.941f, 1.0f, 1.0f), FLinearColor(0.925f, 0.973f, 1.0f, 1.0f), FLinearColor(0.769f, 0.890f, 0.984f, 1.0f), FMargin(10.0f, 8.0f));
		case ETTDActionButtonVariant::Secondary:
		default:
			return MakeStyle(Green(), FLinearColor(0.47f, 0.75f, 0.45f, 1.0f), FLinearColor(0.29f, 0.50f, 0.28f, 1.0f), FMargin(20.0f, 11.0f));
		}
	}

	void ApplyPanel(UBorder* Border, const FLinearColor& Color, const FMargin& Padding)
	{
		if (!Border)
		{
			return;
		}

		Border->SetBrush(MakeBoxBrush(Color));
		Border->SetPadding(Padding);
	}

	void ApplyTopBarPanel(UBorder* Border)
	{
		ApplyPanel(Border, WithAlpha(LightPaper(), 0.90f), FMargin(16.0f, 8.0f));
	}

	void ApplyWoodFramePanel(UBorder* Border, const FMargin& Padding)
	{
		ApplyPanel(Border, DarkWood(), Padding);
	}

	void ApplyPaperPanel(UBorder* Border, const FMargin& Padding)
	{
		ApplyPanel(Border, Paper(), Padding);
	}

	void ApplyCardPanel(UBorder* Border, const FMargin& Padding, const bool bSelected, const bool bEnabled)
	{
		FLinearColor Color = bSelected ? FLinearColor(0.875f, 0.945f, 1.0f, 1.0f) : LightPaper();
		if (!bEnabled)
		{
			Color = WithAlpha(Mix(Color, FLinearColor::Gray, 0.50f), 0.62f);
		}
		ApplyPanel(Border, Color, Padding);
	}

	void ApplySlotPanel(UBorder* Border, const FMargin& Padding, const bool bFilled)
	{
		ApplyPanel(Border, bFilled ? FLinearColor(0.867f, 0.941f, 1.0f, 1.0f) : WithAlpha(FLinearColor::White, 0.36f), Padding);
	}

	void ApplyText(UTextBlock* Text, const int32 Size, const FLinearColor& Color, const ETextJustify::Type Justification, const bool bAutoWrap)
	{
		if (!Text)
		{
			return;
		}

		FSlateFontInfo Font = Text->GetFont();
		Font.Size = Size;
		Text->SetFont(Font);
		Text->SetColorAndOpacity(FSlateColor(Color));
		Text->SetJustification(Justification);
		Text->SetAutoWrapText(bAutoWrap);
		Text->SetLineHeightPercentage(1.08f);
	}

	void ResetFadeIn(UWidget* Widget, float& ElapsedSeconds)
	{
		ElapsedSeconds = 0.0f;
		if (Widget)
		{
			Widget->SetRenderOpacity(0.0f);
		}
	}

	void TickFadeIn(UWidget* Widget, float& ElapsedSeconds, const float DeltaSeconds, const float DurationSeconds)
	{
		if (!Widget)
		{
			return;
		}

		if (ElapsedSeconds >= DurationSeconds)
		{
			return;
		}

		ElapsedSeconds = FMath::Min(ElapsedSeconds + DeltaSeconds, DurationSeconds);
		const float Alpha = DurationSeconds > 0.0f ? ElapsedSeconds / DurationSeconds : 1.0f;
		Widget->SetRenderOpacity(FMath::InterpEaseOut(0.0f, 1.0f, Alpha, 2.0f));
	}
}
