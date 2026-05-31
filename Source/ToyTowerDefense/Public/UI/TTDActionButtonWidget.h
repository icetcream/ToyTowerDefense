#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TTDActionButtonWidget.generated.h"

class UButton;
class UTextBlock;

UCLASS()
class TOYTOWERDEFENSE_API UTTDActionButtonWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Configure(const FText& InLabel, bool bInEnabled, const FSimpleDelegate& InClickHandler);
	void SetLabelAndEnabled(const FText& InLabel, bool bInEnabled);

protected:
	virtual void NativeOnInitialized() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UButton> Button;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> Label;

	FText ButtonLabel;
	bool bButtonEnabled = true;
	FSimpleDelegate ClickHandler;

	UFUNCTION()
	void HandleClicked();

	void ApplyConfiguration();
};
