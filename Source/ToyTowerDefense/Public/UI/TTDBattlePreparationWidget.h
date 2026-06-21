#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TTDBattleTypes.h"
#include "TTDTypes.h"
#include "TTDBattlePreparationWidget.generated.h"

class UTextBlock;
class UVerticalBox;

UCLASS()
class TOYTOWERDEFENSE_API UTTDBattlePreparationWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetLevelId(FName InLevelId);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DiagramCountText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ToyBoxCountText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> PreviewText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> DiagramList;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> ToyBoxList;

	FName LevelId;
	FTTDBattleLevelDefinition LevelDefinition;
	TArray<FTTDCollectionEntry> DiagramEntries;
	TArray<FTTDCollectionEntry> ToyBoxEntries;
	TArray<FName> SelectedDiagramIds;
	TArray<FName> SelectedToyBoxIds;
	TMap<FName, int32> BaseToyBoxStock;
	float EntranceElapsed = 0.0f;
	bool bLayoutBuilt = false;
	bool bDataLoaded = false;

	void BuildLayout();
	void LoadData();
	void RebuildOptionLists();
	void RefreshSummary();
	void HandleBackClicked();
	void HandleStartClicked();
	void ToggleDiagram(FName DiagramId);
	void ToggleToyBox(FName ToyBoxId);
	int32 GetSelectedToyBoxCount(FName ToyBoxId) const;
	int32 GetAvailableToyBoxStock(FName ToyBoxId) const;
	FText BuildEntryPreviewText(const FTTDCollectionEntry& Entry) const;
};
