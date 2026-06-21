#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameplayTagContainer.h"
#include "TTDGameplayMessages.h"
#include "TTDTypes.h"
#include "TTDWarehouseWidget.generated.h"

class UTextBlock;
class UVerticalBox;
class UTTDResearchSubsystem;

UCLASS()
class TOYTOWERDEFENSE_API UTTDWarehouseWidget : public UUserWidget
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

	float EntranceElapsed = 0.0f;
	FGameplayMessageListenerHandle InventoryChangedListenerHandle;
	FGameplayMessageListenerHandle CollectionChangedListenerHandle;

	UTTDResearchSubsystem* GetResearchSubsystem() const;
	void HandleBackClicked();
	void HandleInventoryChangedMessage(FGameplayTag Channel, const FTTDResearchInventoryChangedMessage& Message);
	void HandleCollectionChangedMessage(FGameplayTag Channel, const FTTDResearchCollectionChangedMessage& Message);
	void RefreshContent();
	void AddPartInventorySection(const UTTDResearchSubsystem& ResearchSubsystem);
	void AddCollectionSection(const FText& Title, const TArray<FTTDCollectionEntry>& Entries, const TCHAR* EmptyText);
};
