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
	GameInstance->Shutdown();
	return true;
}

#endif
