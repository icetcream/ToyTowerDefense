#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TTDBattleHUDWidget.generated.h"

class UTextBlock;
class UVerticalBox;

UCLASS()
class TOYTOWERDEFENSE_API UTTDBattleHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetStatusMessage(const FText& Message);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StateText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ProgressText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CastleText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CurrencyText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> InventoryText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SelectedText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> BuildingList;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> ToyBoxList;

	void BuildLayout();
	void BuildActionLists();
	void Refresh();
	FText BuildInventoryText() const;
};
