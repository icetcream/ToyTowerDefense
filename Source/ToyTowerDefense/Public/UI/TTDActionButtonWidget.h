#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TTDActionButtonWidget.generated.h"

class UButton;
class USizeBox;
class UTextBlock;

UENUM(BlueprintType)
enum class ETTDActionButtonVariant : uint8
{
	Primary UMETA(DisplayName = "Primary"),
	Secondary UMETA(DisplayName = "Secondary"),
	MenuCard UMETA(DisplayName = "Menu Card"),
	QueueSlot UMETA(DisplayName = "Queue Slot"),
	Danger UMETA(DisplayName = "Danger"),
	Ghost UMETA(DisplayName = "Ghost"),
	LevelNode UMETA(DisplayName = "Level Node"),
	LevelNodeSelected UMETA(DisplayName = "Selected Level Node"),
	LevelNodeLocked UMETA(DisplayName = "Locked Level Node"),
	LibraryCard UMETA(DisplayName = "Library Card"),
	LibraryCardSelected UMETA(DisplayName = "Selected Library Card"),
	EmptySlot UMETA(DisplayName = "Empty Slot"),
	FilledSlot UMETA(DisplayName = "Filled Slot")
};

UCLASS()
class TOYTOWERDEFENSE_API UTTDActionButtonWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Configure(const FText& InLabel, bool bInEnabled, const FSimpleDelegate& InClickHandler, ETTDActionButtonVariant InVariant = ETTDActionButtonVariant::Secondary);
	void SetLabelAndEnabled(const FText& InLabel, bool bInEnabled);
	void SetVisualVariant(ETTDActionButtonVariant InVariant);
	void SetPulseActive(bool bInPulseActive);
	void SetHoverText(const FText& InHoverText);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<USizeBox> SizeBox;

	UPROPERTY(Transient)
	TObjectPtr<UButton> Button;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> Label;

	FText ButtonLabel;
	FText HoverText;
	bool bButtonEnabled = true;
	bool bHovered = false;
	bool bPressed = false;
	bool bPulseActive = false;
	float CurrentScale = 1.0f;
	float PulseSeconds = 0.0f;
	ETTDActionButtonVariant VisualVariant = ETTDActionButtonVariant::Secondary;
	FSimpleDelegate ClickHandler;

	UFUNCTION()
	void HandleClicked();

	UFUNCTION()
	void HandleHovered();

	UFUNCTION()
	void HandleUnhovered();

	UFUNCTION()
	void HandlePressed();

	UFUNCTION()
	void HandleReleased();

	void ApplyConfiguration();
	void ApplyVariantSizing();
};
