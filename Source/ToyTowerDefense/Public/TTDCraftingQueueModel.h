#pragma once

#include "CoreMinimal.h"
#include "TTDTypes.h"

class TOYTOWERDEFENSE_API FTTDCraftingQueueModel
{
public:
	explicit FTTDCraftingQueueModel(int32 InMaxSlots = 5);

	bool EnqueueToyBox(const FTTDToyBoxDefinition& ToyBox, FTTDCraftQueueItem& OutQueueItem);
	bool ClaimCompletedItem(const FGuid& QueueId, FTTDCraftQueueItem& OutQueueItem);
	void AdvanceTime(float DeltaSeconds);
	int32 SetQueue(const TArray<FTTDCraftQueueItem>& InQueue);

	const TArray<FTTDCraftQueueItem>& GetQueue() const;
	int32 GetMaxSlots() const;
	bool IsFull() const;
	bool HasActiveItems() const;

private:
	TArray<FTTDCraftQueueItem> Queue;
	int32 MaxSlots = 5;
};
