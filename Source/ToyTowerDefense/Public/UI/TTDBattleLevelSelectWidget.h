#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TTDBattleTypes.h"
#include "TTDTypes.h"
#include "TTDBattleLevelSelectWidget.generated.h"

class UCanvasPanel;
class UTextBlock;
class UVerticalBox;

UCLASS()
class TOYTOWERDEFENSE_API UTTDBattleLevelSelectWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	enum class EPanelMode : uint8
	{
		Scout,
		Loadout
	};

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> MapCanvas;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ToyBoxTotalText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> PanelModeText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StageNameText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StageBadgeText;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> PanelBody;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> PanelFooter;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatusText;

	TArray<FTTDBattleLevelDefinition> CachedLevels;
	FTTDBattleLevelDefinition SelectedLevelDefinition;
	TArray<FTTDCollectionEntry> DiagramEntries;
	TArray<FTTDCollectionEntry> ToyBoxEntries;
	TArray<FName> SelectedDiagramIds;
	TArray<FName> SelectedToyBoxIds;
	TMap<FName, int32> BaseToyBoxStock;
	FName SelectedLevelId;
	FName ActiveDiagramTab = TEXT("建筑");
	EPanelMode PanelMode = EPanelMode::Scout;
	float EntranceElapsed = 0.0f;

	void BuildLayout();
	void LoadData();
	void RefreshAll();
	void RebuildMap();
	void RebuildPanel();
	void RenderScoutPanel();
	void RenderLoadoutPanel();
	void ResetTemporaryLoadout();
	void HandleBackClicked();
	void HandleLevelNodeClicked(FName LevelId);
	void HandlePrepareClicked();
	void HandleBackToScoutClicked();
	void HandleUsePresetClicked();
	void HandleStartBattleClicked();
	void HandleDiagramTabClicked(FName TabId);
	void ToggleDiagram(FName DiagramId);
	void PickToyBox(FName ToyBoxId);
	void RemoveDiagramAt(int32 Index);
	void RemoveToyBoxAt(int32 Index);
	bool SelectLevel(FName LevelId);
	bool CanStartBattle() const;
	int32 GetSelectedToyBoxCount(FName ToyBoxId) const;
	int32 GetAvailableToyBoxStock(FName ToyBoxId) const;
	int32 GetTotalToyBoxStock() const;
	FTTDBattleLoadout BuildLoadout() const;
	FText GetLevelDisplayName(const FTTDBattleLevelDefinition& LevelDefinition) const;
	FText BuildRewardPreviewText(const FTTDBattleLevelDefinition& LevelDefinition) const;
	FText BuildLevelInfoText(const FTTDBattleLevelDefinition& LevelDefinition) const;
	FText BuildEntryPartsText(const FTTDCollectionEntry& Entry) const;
	FText BuildToyBoxCardText(const FTTDCollectionEntry& Entry) const;
	const FTTDCollectionEntry* FindDiagramEntry(FName DiagramId) const;
	const FTTDCollectionEntry* FindToyBoxEntry(FName ToyBoxId) const;
};
