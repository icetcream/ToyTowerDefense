#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "TimerManager.h"
#include "TTDGameplayMessages.h"
#include "TTDBattleHUDWidget.generated.h"

class UTextBlock;
class UBorder;
class UVerticalBox;
class UTTDActionButtonWidget;
class UTTDBattleWorldSubsystem;
class ATTDBattleBuildingActor;

struct FTTDToyBoxActionWidgets
{
	TWeakObjectPtr<UTextBlock> SummaryText;
	TWeakObjectPtr<UTTDActionButtonWidget> BuyButton;
	TWeakObjectPtr<UTTDActionButtonWidget> OpenButton;
	int32 PurchaseCost = 0;
};

struct FTTDBuildingCapacityActionWidgets
{
	TWeakObjectPtr<UTextBlock> SummaryText;
	TWeakObjectPtr<UTTDActionButtonWidget> BuyButton;
	int32 PurchaseCost = 0;
};

struct FTTDBuildingLevelCapActionWidgets
{
	TWeakObjectPtr<UTextBlock> SummaryText;
	TWeakObjectPtr<UTTDActionButtonWidget> BuyButton;
	int32 PurchaseCost = 0;
};

struct FTTDBuildingActionWidgets
{
	TWeakObjectPtr<UTTDActionButtonWidget> SelectButton;
	FText DisplayName;
	TArray<FTTDNameStack> PartCosts;
	FName RequiredDiagramId;
};

UCLASS()
class TOYTOWERDEFENSE_API UTTDBattleHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetStatusMessage(const FText& Message);
	void RefreshSelectedBuilding();
	void SelectPlacedBuilding(ATTDBattleBuildingActor* Building);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StateText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ProgressText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> PhaseText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> WaveText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CastleText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CurrencyText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> InventoryText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SelectedText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> BuildingCapacityText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> BuildingLevelCapText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> VictoryOverlay;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> VictoryText;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> BuildingList;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> ToyBoxList;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> BuildingDetailBox;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> BuildingDetailText;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> BuildingUpgradeList;

	UPROPERTY(Transient)
	TObjectPtr<UTTDActionButtonWidget> SellBuildingButton;

	TMap<FName, FTTDToyBoxActionWidgets> ToyBoxActionWidgets;
	TMap<FName, FTTDBuildingActionWidgets> BuildingActionWidgets;
	FTTDBuildingCapacityActionWidgets BuildingCapacityActionWidgets;
	FTTDBuildingLevelCapActionWidgets BuildingLevelCapActionWidgets;
	TWeakObjectPtr<ATTDBattleBuildingActor> SelectedPlacedBuilding;
	TArray<FName> LastActionListDiagramIds;
	float EntranceElapsed = 0.0f;
	FTimerHandle TimedRefreshTimerHandle;
	bool bReturnedAfterVictory = false;
	bool bLastVictoryOverlayVisible = false;
	ETTDBattlePhase LastPhase = ETTDBattlePhase::None;
	int32 LastPhaseRemainingSeconds = INDEX_NONE;
	int32 LastWaveRemainingSeconds = INDEX_NONE;
	int32 LastCurrentWaveNumber = INDEX_NONE;
	int32 LastTotalWaveCount = INDEX_NONE;
	int32 LastVictoryRemainingSeconds = INDEX_NONE;
	FGameplayMessageListenerHandle StateChangedListenerHandle;
	FGameplayMessageListenerHandle PhaseChangedListenerHandle;
	FGameplayMessageListenerHandle ProgressChangedListenerHandle;
	FGameplayMessageListenerHandle CurrencyChangedListenerHandle;
	FGameplayMessageListenerHandle InventoryChangedListenerHandle;
	FGameplayMessageListenerHandle CastleHealthChangedListenerHandle;

	void BuildLayout();
	void BuildActionLists();
	void RegisterMessageListeners();
	void UnregisterMessageListeners();
	void RefreshFromBattleSubsystem();
	void RefreshTimedTexts(bool bForce = false);
	void RefreshTimedTextsFromTimer();
	void RefreshStateText(ETTDBattleState State);
	void RefreshPhaseAndWaveText(const UTTDBattleWorldSubsystem& BattleSubsystem, bool bForce = false);
	void RefreshProgressText(float Progress, float RemainingWeight, float TotalWeight);
	void RefreshCurrencyText(int32 Currency);
	void RefreshBuildingCapacityText(const UTTDBattleWorldSubsystem& BattleSubsystem);
	void RefreshBuildingLevelCapText(const UTTDBattleWorldSubsystem& BattleSubsystem);
	void RefreshBuildingActionStates();
	void RefreshBuildingDetail();
	void RefreshCastleHealthText(float CurrentHealth, float MaxHealth);
	void RefreshActionListStates();
	void RefreshVictoryOverlay(const UTTDBattleWorldSubsystem& BattleSubsystem, bool bForce = false);
	void TryReturnAfterVictory(const UTTDBattleWorldSubsystem& BattleSubsystem);
	void HandleBattleStateChanged(FGameplayTag Channel, const FTTDBattleStateChangedMessage& Message);
	void HandleBattlePhaseChanged(FGameplayTag Channel, const FTTDBattlePhaseChangedMessage& Message);
	void HandleBattleProgressChanged(FGameplayTag Channel, const FTTDBattleProgressChangedMessage& Message);
	void HandleBattleCurrencyChanged(FGameplayTag Channel, const FTTDBattleCurrencyChangedMessage& Message);
	void HandleBattleInventoryChanged(FGameplayTag Channel, const FTTDBattleInventoryChangedMessage& Message);
	void HandleBattleCastleHealthChanged(FGameplayTag Channel, const FTTDBattleCastleHealthChangedMessage& Message);
	FText BuildInventoryText() const;
	FText BuildInventoryText(const TArray<FName>& DiagramIds, const TArray<FTTDNameStack>& Parts, const TArray<FTTDNameStack>& ToyBoxes) const;
	void HandleTestCheatClicked();
	void HandleSellSelectedBuildingClicked();
};
