#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"
#include "Types/SlateEnums.h"

enum class ETTDActionButtonVariant : uint8;
class UBorder;
class UTextBlock;
class UWidget;

namespace TTDUIStyle
{
	FLinearColor Backdrop();
	FLinearColor Paper();
	FLinearColor LightPaper();
	FLinearColor Cardboard();
	FLinearColor Wood();
	FLinearColor DarkWood();
	FLinearColor Ink();
	FLinearColor MutedInk();
	FLinearColor Cream();
	FLinearColor Blue();
	FLinearColor Green();
	FLinearColor Orange();
	FLinearColor Success();
	FLinearColor Warning();
	FLinearColor Danger();
	FLinearColor Line();

	FSlateBrush MakeBoxBrush(const FLinearColor& Color, float Margin = 0.32f);
	FSlateBrush MakeTransparentBrush();
	FButtonStyle MakeButtonStyle(ETTDActionButtonVariant Variant, bool bEnabled);

	void ApplyPanel(UBorder* Border, const FLinearColor& Color, const FMargin& Padding);
	void ApplyTopBarPanel(UBorder* Border);
	void ApplyWoodFramePanel(UBorder* Border, const FMargin& Padding);
	void ApplyPaperPanel(UBorder* Border, const FMargin& Padding);
	void ApplyCardPanel(UBorder* Border, const FMargin& Padding, bool bSelected = false, bool bEnabled = true);
	void ApplySlotPanel(UBorder* Border, const FMargin& Padding, bool bFilled);
	void ApplyText(UTextBlock* Text, int32 Size, const FLinearColor& Color, ETextJustify::Type Justification = ETextJustify::Left, bool bAutoWrap = true);
	void ResetFadeIn(UWidget* Widget, float& ElapsedSeconds);
	void TickFadeIn(UWidget* Widget, float& ElapsedSeconds, float DeltaSeconds, float DurationSeconds = 0.28f);
}
