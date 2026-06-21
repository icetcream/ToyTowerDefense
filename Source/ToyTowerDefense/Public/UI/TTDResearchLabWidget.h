#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameplayTagContainer.h"
#include "TimerManager.h"
#include "TTDGameplayMessages.h"
#include "TTDTypes.h"
#include "TTDResearchLabWidget.generated.h"

class UTextBlock;
class UBorder;
class UVerticalBox;
class UTTDActionButtonWidget;
class UTTDResearchSubsystem;

UCLASS()
class TOYTOWERDEFENSE_API UTTDResearchLabWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> ContentBox;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> FeedbackText;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> DetailOverlay;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTTDActionButtonWidget>> QueueSlotButtons;

	ETTDCollectionCategory ActiveCollectionCategory = ETTDCollectionCategory::Diagram;
	FName SelectedToyBoxId = TEXT("BasicToyBox");
	FText LastFeedback;
	bool bShowingResearch = true;
	bool bSuppressQueueChangedRefresh = false;
	float EntranceElapsed = 0.0f;
	FTimerHandle ResearchQueueRefreshTimerHandle;
	FGameplayMessageListenerHandle QueueChangedListenerHandle;
	FGameplayMessageListenerHandle CollectionChangedListenerHandle;

	UTTDResearchSubsystem* GetResearchSubsystem() const;
	void RegisterMessageListeners();
	void UnregisterMessageListeners();
	void HandleBackClicked();
	void HandleQueueChangedMessage(FGameplayTag Channel, const FTTDResearchQueueChangedMessage& Message);
	void HandleCollectionChangedMessage(FGameplayTag Channel, const FTTDResearchCollectionChangedMessage& Message);
	void HandleInventoryChangedMessage(FGameplayTag Channel, const FTTDResearchInventoryChangedMessage& Message);
	void RenderAlmanac();
	void RenderResearch();
	void RefreshResearchQueueButtons();
	void UpdateResearchQueueRefreshTimer();
	void RenderDiagramResearch();
	void ShowEntryDetails(FTTDCollectionEntry Entry);
	void HideEntryDetails();
	void AddPartScroller(UVerticalBox* ParentBox, const FTTDCollectionEntry& Entry) const;
	FText BuildQueueSlotText(const FTTDCraftQueueItem& QueueItem) const;
	void SetCollectionCategory(ETTDCollectionCategory Category);
	void SetSelectedToyBox(FName ToyBoxId);
	void SetSelectedDiagram(FName DiagramId);
	void EnqueueSelectedToyBox();
	void ClaimQueueItem(FGuid QueueId);
	void ResearchSelectedDiagram();
	FText DescribeEntry(const FTTDCollectionEntry& Entry) const;
	FText FormatSeconds(float Seconds) const;
	FText BuildDiagramCostText(const FTTDDiagramDefinition& Diagram, const UTTDResearchSubsystem& ResearchSubsystem) const;

	FName SelectedDiagramId = NAME_None;
	bool bShowingDiagramResearch = false;
	FGameplayMessageListenerHandle InventoryChangedListenerHandle;
};
