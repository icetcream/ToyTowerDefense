#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameplayTagContainer.h"
#include "TTDGameplayMessages.h"
#include "TTDTypes.h"
#include "TTDResearchLabWidget.generated.h"

class UTextBlock;
class UVerticalBox;
class UTTDActionButtonWidget;
class UTTDResearchSubsystem;

UCLASS()
class TOYTOWERDEFENSE_API UTTDResearchLabWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> ContentBox;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> FeedbackText;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTTDActionButtonWidget>> QueueSlotButtons;

	ETTDCollectionCategory ActiveCollectionCategory = ETTDCollectionCategory::Diagram;
	FName SelectedToyBoxId = TEXT("BasicToyBox");
	FText LastFeedback;
	bool bShowingResearch = true;
	bool bSuppressQueueChangedRefresh = false;
	float ResearchRefreshAccumulator = 0.0f;
	FGameplayMessageListenerHandle QueueChangedListenerHandle;
	FGameplayMessageListenerHandle CollectionChangedListenerHandle;

	UTTDResearchSubsystem* GetResearchSubsystem() const;
	void HandleBackClicked();
	void HandleQueueChangedMessage(FGameplayTag Channel, const FTTDResearchQueueChangedMessage& Message);
	void HandleCollectionChangedMessage(FGameplayTag Channel, const FTTDResearchCollectionChangedMessage& Message);
	void RenderAlmanac();
	void RenderResearch();
	void RefreshResearchQueueButtons();
	void RenderDiagramResearchPlaceholder();
	FText BuildQueueSlotText(const FTTDCraftQueueItem& QueueItem) const;
	void SetCollectionCategory(ETTDCollectionCategory Category);
	void SetSelectedToyBox(FName ToyBoxId);
	void EnqueueSelectedToyBox();
	void ClaimQueueItem(FGuid QueueId);
	FText DescribeEntry(const FTTDCollectionEntry& Entry) const;
	FText FormatSeconds(float Seconds) const;
};
