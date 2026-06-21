#include "TTDCraftingQueueModel.h"

FTTDCraftingQueueModel::FTTDCraftingQueueModel(const int32 InMaxSlots)
	: MaxSlots(FMath::Max(1, InMaxSlots))
{
}

bool FTTDCraftingQueueModel::EnqueueToyBox(const FTTDToyBoxDefinition& ToyBox, FTTDCraftQueueItem& OutQueueItem)
{
	if (IsFull() || ToyBox.ToyBoxId.IsNone())
	{
		return false;
	}

	FTTDCraftQueueItem QueueItem;
	QueueItem.QueueId = FGuid::NewGuid();
	QueueItem.ToyBoxId = ToyBox.ToyBoxId;
	QueueItem.ToyBoxName = ToyBox.DisplayName;
	QueueItem.DurationSeconds = FMath::Max(0.0f, ToyBox.CraftDurationSeconds);
	QueueItem.RemainingSeconds = QueueItem.DurationSeconds;
	QueueItem.bCompleted = QueueItem.RemainingSeconds <= 0.0f;

	Queue.Add(QueueItem);
	OutQueueItem = QueueItem;
	return true;
}

bool FTTDCraftingQueueModel::ClaimCompletedItem(const FGuid& QueueId, FTTDCraftQueueItem& OutQueueItem)
{
	const int32 ItemIndex = Queue.IndexOfByPredicate(
		[&QueueId](const FTTDCraftQueueItem& Item)
		{
			return Item.QueueId == QueueId && Item.bCompleted;
		});
	if (ItemIndex == INDEX_NONE)
	{
		return false;
	}

	OutQueueItem = Queue[ItemIndex];
	Queue.RemoveAt(ItemIndex);
	return true;
}

void FTTDCraftingQueueModel::AdvanceTime(const float DeltaSeconds)
{
	if (DeltaSeconds <= 0.0f)
	{
		return;
	}

	for (FTTDCraftQueueItem& Item : Queue)
	{
		if (Item.bCompleted)
		{
			continue;
		}

		Item.RemainingSeconds = FMath::Max(0.0f, Item.RemainingSeconds - DeltaSeconds);
		Item.bCompleted = Item.RemainingSeconds <= 0.0f;
	}
}

int32 FTTDCraftingQueueModel::SetQueue(const TArray<FTTDCraftQueueItem>& InQueue)
{
	Queue = InQueue;
	const int32 OriginalCount = Queue.Num();
	Queue.RemoveAll(
		[](const FTTDCraftQueueItem& Item)
		{
			return Item.ToyBoxId.IsNone() || !Item.QueueId.IsValid();
		});

	int32 RemovedCount = OriginalCount - Queue.Num();
	if (Queue.Num() > MaxSlots)
	{
		RemovedCount += Queue.Num() - MaxSlots;
		Queue.SetNum(MaxSlots);
	}

	return RemovedCount;
}

const TArray<FTTDCraftQueueItem>& FTTDCraftingQueueModel::GetQueue() const
{
	return Queue;
}

int32 FTTDCraftingQueueModel::GetMaxSlots() const
{
	return MaxSlots;
}

bool FTTDCraftingQueueModel::IsFull() const
{
	return Queue.Num() >= MaxSlots;
}

bool FTTDCraftingQueueModel::HasActiveItems() const
{
	return Queue.ContainsByPredicate(
		[](const FTTDCraftQueueItem& Item)
		{
			return !Item.bCompleted;
		});
}
