#include "Misc/AutomationTest.h"
#include "TTDCraftingQueueModel.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTTDCraftingQueueModelTests,
	"ToyTowerDefense.Research.CraftingQueueModel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTTDCraftingQueueModelTests::RunTest(const FString& Parameters)
{
	FTTDToyBoxDefinition ToyBox;
	ToyBox.ToyBoxId = TEXT("BasicToyBox");
	ToyBox.DisplayName = FText::FromString(TEXT("基础玩具盒"));
	ToyBox.CraftDurationSeconds = 10.0f;
	ToyBox.PossiblePartIds = { TEXT("Gear"), TEXT("Spring") };

	FTTDCraftingQueueModel Queue(2);

	FTTDCraftQueueItem FirstItem;
	FTTDCraftQueueItem SecondItem;
	FTTDCraftQueueItem OverflowItem;

	TestTrue(TEXT("the first toy box can enter an empty queue"), Queue.EnqueueToyBox(ToyBox, FirstItem));
	TestTrue(TEXT("the second toy box can enter the remaining slot"), Queue.EnqueueToyBox(ToyBox, SecondItem));
	TestFalse(TEXT("the queue refuses toy boxes beyond its capacity"), Queue.EnqueueToyBox(ToyBox, OverflowItem));

	Queue.AdvanceTime(4.0f);
	TestEqual(TEXT("the first queue item counts down"), Queue.GetQueue()[0].RemainingSeconds, 6.0f);
	TestEqual(TEXT("all queue items count down at the same time"), Queue.GetQueue()[1].RemainingSeconds, 6.0f);

	Queue.AdvanceTime(6.0f);
	TestTrue(TEXT("the first queue item is completed when time reaches zero"), Queue.GetQueue()[0].bCompleted);
	TestTrue(TEXT("the second queue item is completed when time reaches zero"), Queue.GetQueue()[1].bCompleted);

	FTTDCraftQueueItem ClaimedItem;
	TestTrue(TEXT("a completed item can be claimed by queue id"), Queue.ClaimCompletedItem(FirstItem.QueueId, ClaimedItem));
	TestEqual(TEXT("claiming removes exactly one completed queue item"), Queue.GetQueue().Num(), 1);
	TestEqual(TEXT("the claimed toy box id is preserved"), ClaimedItem.ToyBoxId, ToyBox.ToyBoxId);

	TArray<FTTDCraftQueueItem> OversizedQueue;
	OversizedQueue.Add(FirstItem);
	OversizedQueue.Add(SecondItem);
	OversizedQueue.Add(OverflowItem);
	OversizedQueue.Last().QueueId = FGuid::NewGuid();
	OversizedQueue.Last().ToyBoxId = TEXT("OverflowToyBox");

	TestEqual(TEXT("loading an oversized queue reports trimmed items"), Queue.SetQueue(OversizedQueue), 1);
	TestEqual(TEXT("loading an oversized queue clamps to max slots"), Queue.GetQueue().Num(), 2);

	return true;
}

#endif
