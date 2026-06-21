#include "Misc/AutomationTest.h"

#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "TTDDeveloperSettings.h"
#include "TTDGameplayMessages.h"
#include "TTDGameplayTags.h"
#include "TTDResearchSubsystem.h"
#include "TTDSaveGame.h"
#include "UObject/StrongObjectPtr.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	UDataTable* CreateResearchTestDataTable(UScriptStruct* RowStruct)
	{
		UDataTable* DataTable = NewObject<UDataTable>();
		DataTable->RowStruct = RowStruct;
		return DataTable;
	}

	void AddResearchRows(UDataTable* PartTable, UDataTable* DiagramTable, UDataTable* ToyBoxTable)
	{
		FTTDPartDefinition Gear(TEXT("Gear"), FText::FromString(TEXT("Gear")), FText::FromString(TEXT("Gear part")));
		PartTable->AddRow(TEXT("Gear"), Gear);

		FTTDPartDefinition Spring(TEXT("Spring"), FText::FromString(TEXT("Spring")), FText::FromString(TEXT("Spring part")));
		PartTable->AddRow(TEXT("Spring"), Spring);

		FTTDPartDefinition Battery(TEXT("Battery"), FText::FromString(TEXT("Battery")), FText::FromString(TEXT("Battery part")));
		PartTable->AddRow(TEXT("Battery"), Battery);

		FTTDDiagramDefinition CostedDiagram;
		CostedDiagram.DiagramId = TEXT("CostedDiagram");
		CostedDiagram.DisplayName = FText::FromString(TEXT("Costed Diagram"));
		CostedDiagram.Description = FText::FromString(TEXT("Consumes configured quantities."));
		CostedDiagram.RequiredPartIds = { TEXT("Gear"), TEXT("Spring") };
		CostedDiagram.PartCosts = { FTTDNameStack(TEXT("Gear"), 2), FTTDNameStack(TEXT("Spring"), 1) };
		DiagramTable->AddRow(TEXT("CostedDiagram"), CostedDiagram);

		FTTDDiagramDefinition LegacyDiagram;
		LegacyDiagram.DiagramId = TEXT("LegacyDiagram");
		LegacyDiagram.DisplayName = FText::FromString(TEXT("Legacy Diagram"));
		LegacyDiagram.Description = FText::FromString(TEXT("Falls back to RequiredPartIds."));
		LegacyDiagram.RequiredPartIds = { TEXT("Battery") };
		DiagramTable->AddRow(TEXT("LegacyDiagram"), LegacyDiagram);

		FTTDToyBoxDefinition ToyBox;
		ToyBox.ToyBoxId = TEXT("TestToyBox");
		ToyBox.DisplayName = FText::FromString(TEXT("Test Toy Box"));
		ToyBox.Description = FText::FromString(TEXT("Grants deterministic parts."));
		ToyBox.PossiblePartIds = { TEXT("Gear"), TEXT("Spring") };
		ToyBox.CraftDurationSeconds = 0.0f;
		ToyBoxTable->AddRow(TEXT("TestToyBox"), ToyBox);

		FTTDToyBoxDefinition BatteryBox;
		BatteryBox.ToyBoxId = TEXT("BatteryToyBox");
		BatteryBox.DisplayName = FText::FromString(TEXT("Battery Toy Box"));
		BatteryBox.Description = FText::FromString(TEXT("Grants a battery."));
		BatteryBox.PossiblePartIds = { TEXT("Battery") };
		BatteryBox.CraftDurationSeconds = 0.0f;
		ToyBoxTable->AddRow(TEXT("BatteryToyBox"), BatteryBox);
	}

	struct FTTDResearchTestTables
	{
		TStrongObjectPtr<UDataTable> PartTable;
		TStrongObjectPtr<UDataTable> DiagramTable;
		TStrongObjectPtr<UDataTable> ToyBoxTable;

		FTTDResearchTestTables()
			: PartTable(CreateResearchTestDataTable(FTTDPartDefinition::StaticStruct()))
			, DiagramTable(CreateResearchTestDataTable(FTTDDiagramDefinition::StaticStruct()))
			, ToyBoxTable(CreateResearchTestDataTable(FTTDToyBoxDefinition::StaticStruct()))
		{
			AddResearchRows(PartTable.Get(), DiagramTable.Get(), ToyBoxTable.Get());
		}

		void Configure(UTTDDeveloperSettings* Settings) const
		{
			Settings->GameplayTagDataTables.Reset();
			Settings->GameplayTagDataTables.Add(TAG_TTD_DataTable_Research_Parts, PartTable.Get());
			Settings->GameplayTagDataTables.Add(TAG_TTD_DataTable_Research_Diagrams, DiagramTable.Get());
			Settings->GameplayTagDataTables.Add(TAG_TTD_DataTable_Research_ToyBoxes, ToyBoxTable.Get());
			Settings->CraftingQueueMaxSlots = 5;
			Settings->SaveSlotName = TEXT("ToyTowerDefenseResearchAutomation");
			Settings->DefaultUnlockedDiagramIds.Reset();
			Settings->DefaultUnlockedToyBoxIds = { TEXT("TestToyBox"), TEXT("BatteryToyBox") };
			Settings->DefaultUnlockedPartIds.Reset();
		}
	};

	void RestoreSettings(
		UTTDDeveloperSettings* Settings,
		const TMap<FGameplayTag, TSoftObjectPtr<UDataTable>>& OriginalDataTables,
		const int32 OriginalCraftingQueueMaxSlots,
		const FString& OriginalSaveSlotName,
		const TArray<FName>& OriginalDefaultUnlockedDiagramIds,
		const TArray<FName>& OriginalDefaultUnlockedToyBoxIds,
		const TArray<FName>& OriginalDefaultUnlockedPartIds)
	{
		Settings->GameplayTagDataTables = OriginalDataTables;
		Settings->CraftingQueueMaxSlots = OriginalCraftingQueueMaxSlots;
		Settings->SaveSlotName = OriginalSaveSlotName;
		Settings->DefaultUnlockedDiagramIds = OriginalDefaultUnlockedDiagramIds;
		Settings->DefaultUnlockedToyBoxIds = OriginalDefaultUnlockedToyBoxIds;
		Settings->DefaultUnlockedPartIds = OriginalDefaultUnlockedPartIds;
	}

	int32 FindResearchTestStackCount(const TArray<FTTDNameStack>& Stacks, const FName Id)
	{
		for (const FTTDNameStack& Stack : Stacks)
		{
			if (Stack.Id == Id)
			{
				return Stack.Count;
			}
		}

		return 0;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTTDResearchSubsystemTests,
	"ToyTowerDefense.Research.Subsystem",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTTDResearchSubsystemTests::RunTest(const FString& Parameters)
{
	UTTDDeveloperSettings* Settings = GetMutableDefault<UTTDDeveloperSettings>();
	const TMap<FGameplayTag, TSoftObjectPtr<UDataTable>> OriginalDataTables = Settings->GameplayTagDataTables;
	const int32 OriginalCraftingQueueMaxSlots = Settings->CraftingQueueMaxSlots;
	const FString OriginalSaveSlotName = Settings->SaveSlotName;
	const TArray<FName> OriginalDefaultUnlockedDiagramIds = Settings->DefaultUnlockedDiagramIds;
	const TArray<FName> OriginalDefaultUnlockedToyBoxIds = Settings->DefaultUnlockedToyBoxIds;
	const TArray<FName> OriginalDefaultUnlockedPartIds = Settings->DefaultUnlockedPartIds;

	auto RestoreAll = [Settings, OriginalDataTables, OriginalCraftingQueueMaxSlots, OriginalSaveSlotName, OriginalDefaultUnlockedDiagramIds, OriginalDefaultUnlockedToyBoxIds, OriginalDefaultUnlockedPartIds]()
	{
		RestoreSettings(
			Settings,
			OriginalDataTables,
			OriginalCraftingQueueMaxSlots,
			OriginalSaveSlotName,
			OriginalDefaultUnlockedDiagramIds,
			OriginalDefaultUnlockedToyBoxIds,
			OriginalDefaultUnlockedPartIds);
	};

	const FTTDResearchTestTables Tables;
	Tables.Configure(Settings);
	UGameplayStatics::DeleteGameInSlot(Settings->SaveSlotName, 0);

	{
		UTTDSaveGame* OldSave = Cast<UTTDSaveGame>(UGameplayStatics::CreateSaveGameObject(UTTDSaveGame::StaticClass()));
		OldSave->SaveVersion = 1;
		OldSave->UnlockedDiagramIds = { TEXT("LegacyUnlocked") };
		OldSave->PartInventory = { FTTDNameStack(TEXT("Gear"), 99) };
		TestTrue(TEXT("old version save can be staged"), UGameplayStatics::SaveGameToSlot(OldSave, Settings->SaveSlotName, 0));

		TStrongObjectPtr<UGameInstance> MigrationGameInstance(NewObject<UGameInstance>());
		MigrationGameInstance->Init();
		UTTDResearchSubsystem* MigrationSubsystem = MigrationGameInstance->GetSubsystem<UTTDResearchSubsystem>();
		TestNotNull(TEXT("research subsystem loads for migration check"), MigrationSubsystem);
		if (MigrationSubsystem)
		{
			TestEqual(TEXT("old save inventory is reset"), MigrationSubsystem->GetPartCount(TEXT("Gear")), 0);
			bool bLegacyDiagramStillUnlocked = false;
			for (const FTTDCollectionEntry& Entry : MigrationSubsystem->GetCollectionEntries(ETTDCollectionCategory::Diagram))
			{
				if (Entry.ItemId == TEXT("LegacyUnlocked") && Entry.bUnlocked)
				{
					bLegacyDiagramStillUnlocked = true;
				}
			}
			TestFalse(TEXT("old save unlocked diagrams are not migrated"), bLegacyDiagramStillUnlocked);
		}
		MigrationGameInstance->Shutdown();
	}

	UGameplayStatics::DeleteGameInSlot(Settings->SaveSlotName, 0);

	TStrongObjectPtr<UGameInstance> GameInstance(NewObject<UGameInstance>());
	GameInstance->Init();
	UTTDResearchSubsystem* ResearchSubsystem = GameInstance->GetSubsystem<UTTDResearchSubsystem>();
	if (!TestNotNull(TEXT("research subsystem is available"), ResearchSubsystem))
	{
		GameInstance->Shutdown();
		RestoreAll();
		return false;
	}

	UGameplayMessageSubsystem* MessageSubsystem = GameInstance->GetSubsystem<UGameplayMessageSubsystem>();
	TestNotNull(TEXT("gameplay message subsystem is available"), MessageSubsystem);

	int32 InventoryMessageCount = 0;
	FTTDResearchInventoryChangedMessage LastInventoryMessage;
	FGameplayMessageListenerHandle InventoryHandle = MessageSubsystem->RegisterListener<FTTDResearchInventoryChangedMessage>(
		TAG_TTD_Message_Research_Inventory_Changed,
		[&InventoryMessageCount, &LastInventoryMessage](FGameplayTag Channel, const FTTDResearchInventoryChangedMessage& Payload)
		{
			++InventoryMessageCount;
			LastInventoryMessage = Payload;
		});

	int32 ClaimMessageCount = 0;
	FTTDToyBoxClaimedMessage LastClaimMessage;
	FGameplayMessageListenerHandle ClaimHandle = MessageSubsystem->RegisterListener<FTTDToyBoxClaimedMessage>(
		TAG_TTD_Message_Research_Queue_Claimed,
		[&ClaimMessageCount, &LastClaimMessage](FGameplayTag Channel, const FTTDToyBoxClaimedMessage& Payload)
		{
			++ClaimMessageCount;
			LastClaimMessage = Payload;
		});

	FText FailureReason;
	TestFalse(TEXT("diagram research fails without enough parts"), ResearchSubsystem->ResearchDiagram(TEXT("CostedDiagram"), FailureReason));
	TestTrue(TEXT("insufficient parts provides a failure reason"), !FailureReason.IsEmpty());

	TestTrue(TEXT("test toy box can enter queue"), ResearchSubsystem->EnqueueToyBox(TEXT("TestToyBox"), FailureReason));
	const TArray<FTTDCraftQueueItem> QueueItems = ResearchSubsystem->GetCraftingQueue();
	TestEqual(TEXT("zero-duration toy box is immediately completed"), QueueItems.Num(), 1);
	TestTrue(TEXT("queued zero-duration toy box is completed"), QueueItems.Num() == 1 && QueueItems[0].bCompleted);

	TArray<FTTDPartDefinition> NewParts;
	TestTrue(TEXT("completed toy box can be claimed"), QueueItems.Num() == 1 && ResearchSubsystem->ClaimCompletedToyBox(QueueItems[0].QueueId, NewParts));
	TestEqual(TEXT("claim discovers two parts"), NewParts.Num(), 2);
	TestEqual(TEXT("claim adds gear inventory"), ResearchSubsystem->GetPartCount(TEXT("Gear")), 1);
	TestEqual(TEXT("claim adds spring inventory"), ResearchSubsystem->GetPartCount(TEXT("Spring")), 1);
	TestEqual(TEXT("claim broadcasts inventory"), InventoryMessageCount, 1);
	TestEqual(TEXT("claim broadcasts claimed payload"), ClaimMessageCount, 1);
	TestEqual(TEXT("claim payload includes granted gear"), FindResearchTestStackCount(LastClaimMessage.GrantedParts, TEXT("Gear")), 1);
	TestEqual(TEXT("claim payload includes current spring inventory"), FindResearchTestStackCount(LastClaimMessage.PartInventory, TEXT("Spring")), 1);
	TestEqual(TEXT("inventory payload includes gear"), FindResearchTestStackCount(LastInventoryMessage.PartInventory, TEXT("Gear")), 1);

	TestFalse(TEXT("costed diagram still fails with only one gear"), ResearchSubsystem->ResearchDiagram(TEXT("CostedDiagram"), FailureReason));

	TestTrue(TEXT("second test toy box can enter queue"), ResearchSubsystem->EnqueueToyBox(TEXT("TestToyBox"), FailureReason));
	const TArray<FTTDCraftQueueItem> SecondQueueItems = ResearchSubsystem->GetCraftingQueue();
	TestTrue(TEXT("second completed toy box can be claimed"), SecondQueueItems.Num() == 1 && ResearchSubsystem->ClaimCompletedToyBox(SecondQueueItems[0].QueueId, NewParts));
	TestEqual(TEXT("second claim increments gear inventory"), ResearchSubsystem->GetPartCount(TEXT("Gear")), 2);
	TestEqual(TEXT("second claim increments spring inventory"), ResearchSubsystem->GetPartCount(TEXT("Spring")), 2);

	TestTrue(TEXT("costed diagram succeeds with required parts"), ResearchSubsystem->ResearchDiagram(TEXT("CostedDiagram"), FailureReason));
	TestEqual(TEXT("costed diagram consumes both gears"), ResearchSubsystem->GetPartCount(TEXT("Gear")), 0);
	TestEqual(TEXT("costed diagram consumes one spring"), ResearchSubsystem->GetPartCount(TEXT("Spring")), 1);
	TestFalse(TEXT("already researched diagram cannot be researched again"), ResearchSubsystem->ResearchDiagram(TEXT("CostedDiagram"), FailureReason));

	TestTrue(TEXT("battery toy box can enter queue"), ResearchSubsystem->EnqueueToyBox(TEXT("BatteryToyBox"), FailureReason));
	const TArray<FTTDCraftQueueItem> BatteryQueueItems = ResearchSubsystem->GetCraftingQueue();
	TestTrue(TEXT("battery toy box can be claimed"), BatteryQueueItems.Num() == 1 && ResearchSubsystem->ClaimCompletedToyBox(BatteryQueueItems[0].QueueId, NewParts));
	TestEqual(TEXT("battery inventory is added"), ResearchSubsystem->GetPartCount(TEXT("Battery")), 1);
	TestTrue(TEXT("legacy RequiredPartIds fallback succeeds"), ResearchSubsystem->ResearchDiagram(TEXT("LegacyDiagram"), FailureReason));
	TestEqual(TEXT("legacy fallback consumes one battery"), ResearchSubsystem->GetPartCount(TEXT("Battery")), 0);

	ClaimHandle.Unregister();
	InventoryHandle.Unregister();
	GameInstance->Shutdown();
	UGameplayStatics::DeleteGameInSlot(Settings->SaveSlotName, 0);
	RestoreAll();
	return true;
}

#endif
