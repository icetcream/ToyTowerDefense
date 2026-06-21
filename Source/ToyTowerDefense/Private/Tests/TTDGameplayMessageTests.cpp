#include "Misc/AutomationTest.h"

#include "Engine/GameInstance.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "TTDGameplayMessages.h"
#include "UObject/StrongObjectPtr.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTTDGameplayMessageTests,
	"ToyTowerDefense.Messaging.GameplayMessageRouter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTTDGameplayMessageTests::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UGameInstance> GameInstance(NewObject<UGameInstance>());
	GameInstance->Init();

	UGameplayMessageSubsystem* MessageSubsystem = GameInstance->GetSubsystem<UGameplayMessageSubsystem>();
	TestNotNull(TEXT("gameplay message subsystem is available"), MessageSubsystem);

	int32 ReceivedCount = 0;
	FTTDResearchQueueChangedMessage ReceivedMessage;

	FGameplayMessageListenerHandle Handle = MessageSubsystem->RegisterListener<FTTDResearchQueueChangedMessage>(
		TAG_TTD_Message_Research_Queue_Changed,
		[&ReceivedCount, &ReceivedMessage](FGameplayTag Channel, const FTTDResearchQueueChangedMessage& Payload)
		{
			++ReceivedCount;
			ReceivedMessage = Payload;
		});

	FTTDResearchQueueChangedMessage Message;
	Message.MaxSlots = 5;
	Message.QueueItems.AddDefaulted();

	MessageSubsystem->BroadcastMessage(TAG_TTD_Message_Research_Queue_Changed, Message);

	TestEqual(TEXT("listener receives the queue changed payload"), ReceivedCount, 1);
	TestEqual(TEXT("payload max slots survives routing"), ReceivedMessage.MaxSlots, 5);
	TestEqual(TEXT("payload queue items survive routing"), ReceivedMessage.QueueItems.Num(), 1);

	Handle.Unregister();

	int32 InventoryReceivedCount = 0;
	FTTDResearchInventoryChangedMessage ReceivedInventoryMessage;
	FGameplayMessageListenerHandle InventoryHandle = MessageSubsystem->RegisterListener<FTTDResearchInventoryChangedMessage>(
		TAG_TTD_Message_Research_Inventory_Changed,
		[&InventoryReceivedCount, &ReceivedInventoryMessage](FGameplayTag Channel, const FTTDResearchInventoryChangedMessage& Payload)
		{
			++InventoryReceivedCount;
			ReceivedInventoryMessage = Payload;
		});

	FTTDResearchInventoryChangedMessage InventoryMessage;
	InventoryMessage.PartInventory = { FTTDNameStack(TEXT("Gear"), 3) };

	MessageSubsystem->BroadcastMessage(TAG_TTD_Message_Research_Inventory_Changed, InventoryMessage);

	TestEqual(TEXT("listener receives the research inventory payload"), InventoryReceivedCount, 1);
	TestEqual(TEXT("payload inventory survives routing"), ReceivedInventoryMessage.PartInventory.Num(), 1);
	TestEqual(TEXT("payload inventory count survives routing"), ReceivedInventoryMessage.PartInventory[0].Count, 3);

	InventoryHandle.Unregister();

	int32 CastleHealthReceivedCount = 0;
	FTTDBattleCastleHealthChangedMessage ReceivedCastleHealthMessage;
	FGameplayMessageListenerHandle CastleHealthHandle = MessageSubsystem->RegisterListener<FTTDBattleCastleHealthChangedMessage>(
		TAG_TTD_Message_Battle_CastleHealth_Changed,
		[&CastleHealthReceivedCount, &ReceivedCastleHealthMessage](FGameplayTag Channel, const FTTDBattleCastleHealthChangedMessage& Payload)
		{
			++CastleHealthReceivedCount;
			ReceivedCastleHealthMessage = Payload;
		});

	FTTDBattleCastleHealthChangedMessage CastleHealthMessage;
	CastleHealthMessage.CurrentHealth = 125.0f;
	CastleHealthMessage.MaxHealth = 500.0f;

	MessageSubsystem->BroadcastMessage(TAG_TTD_Message_Battle_CastleHealth_Changed, CastleHealthMessage);

	TestEqual(TEXT("listener receives the castle health payload"), CastleHealthReceivedCount, 1);
	TestEqual(TEXT("payload current health survives routing"), ReceivedCastleHealthMessage.CurrentHealth, 125.0f);
	TestEqual(TEXT("payload max health survives routing"), ReceivedCastleHealthMessage.MaxHealth, 500.0f);

	CastleHealthHandle.Unregister();
	GameInstance->Shutdown();
	return true;
}

#endif
