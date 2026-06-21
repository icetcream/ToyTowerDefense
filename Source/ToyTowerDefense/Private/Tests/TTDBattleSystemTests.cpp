#include "Misc/AutomationTest.h"

#include "Battle/TTDBattleBackgroundActor.h"
#include "Battle/TTDBattleBuildingActor.h"
#include "Battle/TTDBattleCastleActor.h"
#include "Battle/TTDBattleEnemyActor.h"
#include "Battle/TTDBattleProjectileActor.h"
#include "Battle/TTDBattleSpawnPointActor.h"
#include "Battle/TTDBattleTargetInterface.h"
#include "Battle/TTDBuildSlotActor.h"
#include "Engine/DataTable.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "TTDBattleWorldSubsystem.h"
#include "TTDDeveloperSettings.h"
#include "TTDGameplayTags.h"
#include "TTDObjectPoolTypes.h"
#include "UObject/StrongObjectPtr.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	UDataTable* CreateBattleTestDataTable(UScriptStruct* RowStruct)
	{
		UDataTable* DataTable = NewObject<UDataTable>();
		DataTable->RowStruct = RowStruct;
		return DataTable;
	}

	void AddBattleRows(
		UDataTable* LevelTable,
		UDataTable* WaveTable,
		UDataTable* EnemyTable,
		UDataTable* BuildingTable,
		UDataTable* ToyBoxRewardTable,
		UDataTable* HeightEffectTable)
	{
		FTTDBattleLevelDefinition Level;
		Level.LevelId = TEXT("TestLevel");
		Level.CastleClass = ATTDBattleCastleActor::StaticClass();
		Level.CastleTransform = FTransform::Identity;
		Level.CastleMaxHealth = 500.0f;
		Level.WaveIds = { TEXT("WaveA") };
		Level.StartingDiagramIds = { TEXT("BasicTower"), TEXT("ProjectileTower") };
		Level.StartingParts = { FTTDNameStack(TEXT("Gear"), 5), FTTDNameStack(TEXT("Spring"), 3) };
		Level.StartingToyBoxes = { FTTDNameStack(TEXT("BasicToyBox"), 1) };
		Level.StartingCurrency = 10;
		Level.StartingBuildingCapacity = 3;
		Level.BuildingCapacityPurchaseCost = 5;
		Level.StartingBuildingLevelCap = 1;
		Level.BuildingLevelCapPurchaseCost = 4;
		Level.MaxSelectedDiagrams = 2;
		Level.MaxSelectedToyBoxes = 2;
		Level.PreparationDurationSeconds = 0.2f;
		Level.VictoryReturnDelaySeconds = 0.5f;
		Level.Background.ArenaHalfExtent = FVector2D(1200.0f, 900.0f);
		LevelTable->AddRow(TEXT("TestLevel"), Level);

		FTTDWaveDefinition Wave;
		Wave.WaveId = TEXT("WaveA");
		Wave.DelayBeforeWave = 0.1f;
		Wave.WaveDurationSeconds = 0.5f;
		FTTDWaveEnemyEntry EnemyEntry;
		EnemyEntry.EnemyId = TEXT("BasicEnemy");
		EnemyEntry.Count = 2;
		EnemyEntry.SpawnInterval = 0.1f;
		EnemyEntry.SpawnGroup = TEXT("Edge");
		Wave.EnemyEntries = { EnemyEntry };
		WaveTable->AddRow(TEXT("WaveA"), Wave);

		FTTDEnemyDefinition Enemy;
		Enemy.EnemyId = TEXT("BasicEnemy");
		Enemy.DisplayName = FText::FromString(TEXT("基础敌人"));
		Enemy.EnemyClass = ATTDBattleEnemyActor::StaticClass();
		Enemy.MaxHealth = 10.0f;
		Enemy.MoveSpeed = 0.0f;
		Enemy.AttackDamage = 0.0f;
		Enemy.ProgressWeight = 2.0f;
		Enemy.CurrencyDrop = 3;
		EnemyTable->AddRow(TEXT("BasicEnemy"), Enemy);

		FTTDBuildingDefinition BasicTower;
		BasicTower.BuildingId = TEXT("BasicTower");
		BasicTower.DisplayName = FText::FromString(TEXT("基础塔"));
		BasicTower.RequiredDiagramId = TEXT("BasicTower");
		BasicTower.BuildingClass = ATTDBattleBuildingActor::StaticClass();
		BasicTower.PartCosts = { FTTDNameStack(TEXT("Gear"), 2) };
		BasicTower.AttackMode = ETTDBuildingAttackMode::InstantDamage;
		BasicTower.BaseStats.AttackDamage = 5.0f;
		BasicTower.BaseStats.AttackRange = 1000.0f;
		BasicTower.BaseStats.AttackInterval = 0.5f;
		BasicTower.BaseStats.MaxHealth = 50.0f;
		FTTDBattleBuildingUpgradeOption GearUpgrade;
		GearUpgrade.PartId = TEXT("Gear");
		GearUpgrade.PartCost = 1;
		GearUpgrade.Modifiers = { { ETTDBattleAttribute::AttackDamage, ETTDAttributeModifierOp::Add, 3.0f } };
		BasicTower.UpgradeOptions = { GearUpgrade };
		BuildingTable->AddRow(TEXT("BasicTower"), BasicTower);

		FTTDBuildingDefinition MissingDiagramTower = BasicTower;
		MissingDiagramTower.BuildingId = TEXT("MissingDiagramTower");
		MissingDiagramTower.DisplayName = FText::FromString(TEXT("缺图纸测试塔"));
		MissingDiagramTower.RequiredDiagramId = TEXT("MissingDiagram");
		BuildingTable->AddRow(TEXT("MissingDiagramTower"), MissingDiagramTower);

		FTTDBuildingDefinition ExpensiveTower = BasicTower;
		ExpensiveTower.BuildingId = TEXT("ExpensiveTower");
		ExpensiveTower.DisplayName = FText::FromString(TEXT("高耗材测试塔"));
		ExpensiveTower.PartCosts = { FTTDNameStack(TEXT("Gear"), 999) };
		BuildingTable->AddRow(TEXT("ExpensiveTower"), ExpensiveTower);

		FTTDBuildingDefinition ProjectileTower = BasicTower;
		ProjectileTower.BuildingId = TEXT("ProjectileTower");
		ProjectileTower.DisplayName = FText::FromString(TEXT("弹射塔"));
		ProjectileTower.RequiredDiagramId = TEXT("ProjectileTower");
		ProjectileTower.PartCosts = { FTTDNameStack(TEXT("Gear"), 1), FTTDNameStack(TEXT("Spring"), 1) };
		ProjectileTower.AttackMode = ETTDBuildingAttackMode::Projectile;
		ProjectileTower.BaseStats.AttackDamage = 10.0f;
		ProjectileTower.BaseStats.AttackRange = 1000.0f;
		ProjectileTower.BaseStats.AttackInterval = 0.01f;
		ProjectileTower.BaseStats.ProjectileSpeed = 3000.0f;
		ProjectileTower.ProjectileClass = ATTDBattleProjectileActor::StaticClass();
		BuildingTable->AddRow(TEXT("ProjectileTower"), ProjectileTower);

		FTTDToyBoxRewardDefinition ToyBox;
		ToyBox.ToyBoxId = TEXT("BasicToyBox");
		ToyBox.DisplayName = FText::FromString(TEXT("基础玩具盒"));
		ToyBox.PurchaseCost = 5;
		ToyBox.RollCount = 2;
		FTTDToyBoxRewardEntry Reward;
		Reward.PartId = TEXT("Gear");
		Reward.Weight = 1.0f;
		Reward.MinQuantity = 1;
		Reward.MaxQuantity = 1;
		ToyBox.RewardEntries = { Reward };
		ToyBoxRewardTable->AddRow(TEXT("BasicToyBox"), ToyBox);

		FTTDBattleHeightEffectDefinition HeightEffect;
		HeightEffect.HeightEffectId = TEXT("HighGround");
		HeightEffect.Modifiers = {
			{ ETTDBattleAttribute::AttackDamage, ETTDAttributeModifierOp::Add, 5.0f },
			{ ETTDBattleAttribute::AttackRange, ETTDAttributeModifierOp::Multiply, 2.0f },
			{ ETTDBattleAttribute::AttackInterval, ETTDAttributeModifierOp::Override, 0.25f },
			{ ETTDBattleAttribute::MoveSpeed, ETTDAttributeModifierOp::Override, 12.0f }
		};
		HeightEffectTable->AddRow(TEXT("HighGround"), HeightEffect);
	}

	void ConfigureBattleTables(
		UTTDDeveloperSettings* Settings,
		UDataTable* LevelTable,
		UDataTable* WaveTable,
		UDataTable* EnemyTable,
		UDataTable* BuildingTable,
		UDataTable* ToyBoxRewardTable,
		UDataTable* HeightEffectTable)
	{
		Settings->GameplayTagDataTables.Add(TAG_TTD_DataTable_Battle_Levels, LevelTable);
		Settings->GameplayTagDataTables.Add(TAG_TTD_DataTable_Battle_Waves, WaveTable);
		Settings->GameplayTagDataTables.Add(TAG_TTD_DataTable_Battle_Enemies, EnemyTable);
		Settings->GameplayTagDataTables.Add(TAG_TTD_DataTable_Battle_Buildings, BuildingTable);
		Settings->GameplayTagDataTables.Add(TAG_TTD_DataTable_Battle_ToyBoxRewards, ToyBoxRewardTable);
		Settings->GameplayTagDataTables.Add(TAG_TTD_DataTable_Battle_HeightEffects, HeightEffectTable);
		Settings->DefaultBattleLevelId = TEXT("TestLevel");
	}

	void ConfigureObjectPoolTable(UTTDDeveloperSettings* Settings, UDataTable* ObjectPoolTable)
	{
		Settings->GameplayTagDataTables.Add(TAG_TTD_DataTable_ObjectPool_Definitions, ObjectPoolTable);
	}

	void ResetSettingsForBattleTest(UTTDDeveloperSettings* Settings)
	{
		Settings->GameplayTagDataTables.Reset();
		Settings->DefaultBattleLevelId = NAME_None;
	}

	UDataTable* CreateBattleObjectPoolDataTable()
	{
		return CreateBattleTestDataTable(FTTDObjectPoolDefinition::StaticStruct());
	}

	struct FTTDBattleTestTables
	{
		TStrongObjectPtr<UDataTable> LevelTable;
		TStrongObjectPtr<UDataTable> WaveTable;
		TStrongObjectPtr<UDataTable> EnemyTable;
		TStrongObjectPtr<UDataTable> BuildingTable;
		TStrongObjectPtr<UDataTable> ToyBoxRewardTable;
		TStrongObjectPtr<UDataTable> HeightEffectTable;

		FTTDBattleTestTables()
			: LevelTable(CreateBattleTestDataTable(FTTDBattleLevelDefinition::StaticStruct()))
			, WaveTable(CreateBattleTestDataTable(FTTDWaveDefinition::StaticStruct()))
			, EnemyTable(CreateBattleTestDataTable(FTTDEnemyDefinition::StaticStruct()))
			, BuildingTable(CreateBattleTestDataTable(FTTDBuildingDefinition::StaticStruct()))
			, ToyBoxRewardTable(CreateBattleTestDataTable(FTTDToyBoxRewardDefinition::StaticStruct()))
			, HeightEffectTable(CreateBattleTestDataTable(FTTDBattleHeightEffectDefinition::StaticStruct()))
		{
		}

		void AddDefaultRows() const
		{
			AddBattleRows(LevelTable.Get(), WaveTable.Get(), EnemyTable.Get(), BuildingTable.Get(), ToyBoxRewardTable.Get(), HeightEffectTable.Get());
		}

		void Configure(UTTDDeveloperSettings* Settings) const
		{
			ConfigureBattleTables(Settings, LevelTable.Get(), WaveTable.Get(), EnemyTable.Get(), BuildingTable.Get(), ToyBoxRewardTable.Get(), HeightEffectTable.Get());
		}
	};

	void SpawnRequiredBattleActors(UWorld* World, ATTDBattleSpawnPointActor*& OutSpawnPoint, ATTDBuildSlotActor*& OutSlot)
	{
		OutSpawnPoint = World->SpawnActor<ATTDBattleSpawnPointActor>(FVector(300.0f, 0.0f, 0.0f), FRotator::ZeroRotator);
		if (OutSpawnPoint)
		{
			OutSpawnPoint->ConfigureSpawnPoint(TEXT("Edge"));
		}

		OutSlot = World->SpawnActor<ATTDBuildSlotActor>(FVector(0.0f, 100.0f, 0.0f), FRotator::ZeroRotator);
		if (OutSlot)
		{
			OutSlot->ConfigureSlot(TEXT("Slot"), FIntPoint::ZeroValue, 0, NAME_None);
		}
	}

	UWorld* CreateBattleTestWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, FName(TEXT("TTDBattleTestWorld")));
		if (!World)
		{
			return nullptr;
		}

		FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
		WorldContext.SetCurrentWorld(World);
		return World;
	}

	void DestroyBattleTestWorld(UWorld* World)
	{
		if (!World)
		{
			return;
		}

		World->DestroyWorld(false);
		if (GEngine)
		{
			GEngine->DestroyWorldContext(World);
		}
	}

	ATTDBuildSlotActor* SpawnSlot(UWorld* World, const FName SlotId, const FVector& Location, const FName HeightEffectId = NAME_None)
	{
		ATTDBuildSlotActor* Slot = World->SpawnActor<ATTDBuildSlotActor>(Location, FRotator::ZeroRotator);
		if (Slot)
		{
			Slot->ConfigureSlot(SlotId, FIntPoint::ZeroValue, HeightEffectId.IsNone() ? 0 : 5, HeightEffectId);
		}
		return Slot;
	}

	bool ContainsBuildingDefinition(const TArray<FTTDBuildingDefinition>& BuildingDefinitions, const FName BuildingId)
	{
		return BuildingDefinitions.ContainsByPredicate(
			[BuildingId](const FTTDBuildingDefinition& BuildingDefinition)
			{
				return BuildingDefinition.BuildingId == BuildingId;
			});
	}

	int32 FindBattleTestStackCount(const TArray<FTTDNameStack>& Stacks, const FName Id)
	{
		for (const FTTDNameStack& Stack : Stacks)
		{
			if (Stack.Id == Id)
			{
				return FMath::Max(0, Stack.Count);
			}
		}
		return 0;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTTDBattleSystemTests,
	"ToyTowerDefense.Battle.WorldSubsystem",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTTDBattleSystemTests::RunTest(const FString& Parameters)
{
	UTTDDeveloperSettings* Settings = GetMutableDefault<UTTDDeveloperSettings>();
	const TMap<FGameplayTag, TSoftObjectPtr<UDataTable>> OriginalDataTables = Settings->GameplayTagDataTables;
	const FName OriginalDefaultBattleLevelId = Settings->DefaultBattleLevelId;
	const bool bOriginalShowBattleTestCheatButton = Settings->bShowBattleTestCheatButton;

	auto RestoreSettings = [Settings, OriginalDataTables, OriginalDefaultBattleLevelId, bOriginalShowBattleTestCheatButton]()
	{
		Settings->GameplayTagDataTables = OriginalDataTables;
		Settings->DefaultBattleLevelId = OriginalDefaultBattleLevelId;
		Settings->bShowBattleTestCheatButton = bOriginalShowBattleTestCheatButton;
	};

	auto HasFailureText = [](const FText& FailureReason)
	{
		return !FailureReason.ToString().IsEmpty();
	};

	{
		ResetSettingsForBattleTest(Settings);
		Settings->DefaultBattleLevelId = TEXT("MissingLevel");

		UWorld* MissingTablesWorld = CreateBattleTestWorld();
		if (TestNotNull(TEXT("missing tables test world is created"), MissingTablesWorld))
		{
			ATTDBattleSpawnPointActor* SpawnPoint = nullptr;
			ATTDBuildSlotActor* Slot = nullptr;
			SpawnRequiredBattleActors(MissingTablesWorld, SpawnPoint, Slot);

			UTTDBattleWorldSubsystem* BattleSubsystem = MissingTablesWorld->GetSubsystem<UTTDBattleWorldSubsystem>();
			TestNotNull(TEXT("missing tables battle subsystem is available"), BattleSubsystem);
			if (BattleSubsystem)
			{
				FText FailureReason;
				TestFalse(TEXT("missing battle data tables or default level id fail explicitly"), BattleSubsystem->StartDefaultBattle(FailureReason));
				TestTrue(TEXT("missing battle data tables provide a failure reason"), HasFailureText(FailureReason));
				TestEqual(TEXT("missing battle data tables do not enter a fake battle"), BattleSubsystem->GetBattleState(), ETTDBattleState::Inactive);
			}

			DestroyBattleTestWorld(MissingTablesWorld);
		}
	}

	{
		ResetSettingsForBattleTest(Settings);
		FTTDBattleTestTables Tables;
		Tables.AddDefaultRows();
		Tables.Configure(Settings);

		UWorld* NoSpawnWorld = CreateBattleTestWorld();
		if (TestNotNull(TEXT("no spawn point test world is created"), NoSpawnWorld))
		{
			SpawnSlot(NoSpawnWorld, TEXT("OnlySlot"), FVector::ZeroVector);

			UTTDBattleWorldSubsystem* BattleSubsystem = NoSpawnWorld->GetSubsystem<UTTDBattleWorldSubsystem>();
			TestNotNull(TEXT("no spawn point battle subsystem is available"), BattleSubsystem);
			if (BattleSubsystem)
			{
				FText FailureReason;
				TestFalse(TEXT("battle start fails when no spawn point exists"), BattleSubsystem->StartDefaultBattle(FailureReason));
				TestTrue(TEXT("no spawn point failure provides a reason"), HasFailureText(FailureReason));
				TestEqual(TEXT("no spawn point failure leaves battle inactive"), BattleSubsystem->GetBattleState(), ETTDBattleState::Inactive);
			}

			DestroyBattleTestWorld(NoSpawnWorld);
		}
	}

	{
		ResetSettingsForBattleTest(Settings);
		FTTDBattleTestTables Tables;
		Tables.AddDefaultRows();
		if (FTTDBattleLevelDefinition* Level = Tables.LevelTable->FindRow<FTTDBattleLevelDefinition>(TEXT("TestLevel"), TEXT("NoSlotStartupValidation")))
		{
			Level->bAllowArbitraryBuildPlacement = false;
		}
		Tables.Configure(Settings);

		UWorld* NoSlotWorld = CreateBattleTestWorld();
		if (TestNotNull(TEXT("no build slot test world is created"), NoSlotWorld))
		{
			ATTDBattleSpawnPointActor* SpawnPoint = NoSlotWorld->SpawnActor<ATTDBattleSpawnPointActor>(FVector(300.0f, 0.0f, 0.0f), FRotator::ZeroRotator);
			if (SpawnPoint)
			{
				SpawnPoint->ConfigureSpawnPoint(TEXT("Edge"));
			}

			UTTDBattleWorldSubsystem* BattleSubsystem = NoSlotWorld->GetSubsystem<UTTDBattleWorldSubsystem>();
			TestNotNull(TEXT("no build slot battle subsystem is available"), BattleSubsystem);
			if (BattleSubsystem)
			{
				FText FailureReason;
				TestFalse(TEXT("battle start fails when no build slot exists"), BattleSubsystem->StartDefaultBattle(FailureReason));
				TestTrue(TEXT("no build slot failure provides a reason"), HasFailureText(FailureReason));
				TestEqual(TEXT("no build slot failure leaves battle inactive"), BattleSubsystem->GetBattleState(), ETTDBattleState::Inactive);
			}

			DestroyBattleTestWorld(NoSlotWorld);
		}
	}

	{
		ResetSettingsForBattleTest(Settings);
		FTTDBattleTestTables Tables;
		Tables.AddDefaultRows();
		if (FTTDBattleLevelDefinition* Level = Tables.LevelTable->FindRow<FTTDBattleLevelDefinition>(TEXT("TestLevel"), TEXT("ArbitraryPlacementNoSlotStartupValidation")))
		{
			Level->bAllowArbitraryBuildPlacement = true;
		}
		Tables.Configure(Settings);

		UWorld* ArbitraryNoSlotWorld = CreateBattleTestWorld();
		if (TestNotNull(TEXT("arbitrary placement no slot test world is created"), ArbitraryNoSlotWorld))
		{
			ATTDBattleSpawnPointActor* SpawnPoint = ArbitraryNoSlotWorld->SpawnActor<ATTDBattleSpawnPointActor>(FVector(300.0f, 0.0f, 0.0f), FRotator::ZeroRotator);
			if (SpawnPoint)
			{
				SpawnPoint->ConfigureSpawnPoint(TEXT("Edge"));
			}

			UTTDBattleWorldSubsystem* BattleSubsystem = ArbitraryNoSlotWorld->GetSubsystem<UTTDBattleWorldSubsystem>();
			TestNotNull(TEXT("arbitrary placement no slot battle subsystem is available"), BattleSubsystem);
			if (BattleSubsystem)
			{
				FText FailureReason;
				TestTrue(TEXT("battle can start without placed slots when arbitrary placement is enabled"), BattleSubsystem->StartDefaultBattle(FailureReason));
				TestTrue(TEXT("arbitrary placement can build without placed slots"), BattleSubsystem->TryBuildAtLocation(TEXT("BasicTower"), FVector(300.0f, 300.0f, 0.0f), FailureReason));
			}

			DestroyBattleTestWorld(ArbitraryNoSlotWorld);
		}
	}

	{
		ResetSettingsForBattleTest(Settings);
		FTTDBattleTestTables Tables;
		Tables.AddDefaultRows();
		if (FTTDWaveDefinition* Wave = Tables.WaveTable->FindRow<FTTDWaveDefinition>(TEXT("WaveA"), TEXT("MissingEnemyStartupValidation")))
		{
			if (!Wave->EnemyEntries.IsEmpty())
			{
				Wave->EnemyEntries[0].EnemyId = TEXT("MissingEnemy");
			}
		}
		Tables.Configure(Settings);

		UWorld* MissingEnemyWorld = CreateBattleTestWorld();
		if (TestNotNull(TEXT("missing enemy test world is created"), MissingEnemyWorld))
		{
			ATTDBattleSpawnPointActor* SpawnPoint = nullptr;
			ATTDBuildSlotActor* Slot = nullptr;
			SpawnRequiredBattleActors(MissingEnemyWorld, SpawnPoint, Slot);

			UTTDBattleWorldSubsystem* BattleSubsystem = MissingEnemyWorld->GetSubsystem<UTTDBattleWorldSubsystem>();
			TestNotNull(TEXT("missing enemy battle subsystem is available"), BattleSubsystem);
			if (BattleSubsystem)
			{
				FText FailureReason;
				TestFalse(TEXT("battle start fails when a wave references a missing enemy"), BattleSubsystem->StartDefaultBattle(FailureReason));
				TestTrue(TEXT("missing enemy failure provides a reason"), HasFailureText(FailureReason));
				TestEqual(TEXT("missing enemy failure leaves battle inactive"), BattleSubsystem->GetBattleState(), ETTDBattleState::Inactive);
			}

			DestroyBattleTestWorld(MissingEnemyWorld);
		}
	}

	{
		ResetSettingsForBattleTest(Settings);
		FTTDBattleTestTables Tables;
		Tables.AddDefaultRows();
		if (FTTDBattleLevelDefinition* Level = Tables.LevelTable->FindRow<FTTDBattleLevelDefinition>(TEXT("TestLevel"), TEXT("InvalidBackgroundStartupValidation")))
		{
			Level->Background.BackgroundClass = AActor::StaticClass();
		}
		Tables.Configure(Settings);

		UWorld* InvalidBackgroundWorld = CreateBattleTestWorld();
		if (TestNotNull(TEXT("invalid background test world is created"), InvalidBackgroundWorld))
		{
			ATTDBattleSpawnPointActor* SpawnPoint = nullptr;
			ATTDBuildSlotActor* Slot = nullptr;
			SpawnRequiredBattleActors(InvalidBackgroundWorld, SpawnPoint, Slot);

			UTTDBattleWorldSubsystem* BattleSubsystem = InvalidBackgroundWorld->GetSubsystem<UTTDBattleWorldSubsystem>();
			TestNotNull(TEXT("invalid background battle subsystem is available"), BattleSubsystem);
			if (BattleSubsystem)
			{
				FText FailureReason;
				TestFalse(TEXT("battle start fails when background class is invalid"), BattleSubsystem->StartDefaultBattle(FailureReason));
				TestTrue(TEXT("invalid background failure provides a reason"), HasFailureText(FailureReason));
				TestEqual(TEXT("invalid background failure leaves battle inactive"), BattleSubsystem->GetBattleState(), ETTDBattleState::Inactive);
			}

			DestroyBattleTestWorld(InvalidBackgroundWorld);
		}
	}

	{
		ResetSettingsForBattleTest(Settings);
		FTTDBattleTestTables Tables;
		Tables.AddDefaultRows();
		if (FTTDEnemyDefinition* Enemy = Tables.EnemyTable->FindRow<FTTDEnemyDefinition>(TEXT("BasicEnemy"), TEXT("NonPoolableEnemyStartupValidation")))
		{
			Enemy->EnemyClass = AActor::StaticClass();
		}
		Tables.Configure(Settings);

		UWorld* NonPoolableEnemyWorld = CreateBattleTestWorld();
		if (TestNotNull(TEXT("non-poolable enemy test world is created"), NonPoolableEnemyWorld))
		{
			ATTDBattleSpawnPointActor* SpawnPoint = nullptr;
			ATTDBuildSlotActor* Slot = nullptr;
			SpawnRequiredBattleActors(NonPoolableEnemyWorld, SpawnPoint, Slot);

			UTTDBattleWorldSubsystem* BattleSubsystem = NonPoolableEnemyWorld->GetSubsystem<UTTDBattleWorldSubsystem>();
			TestNotNull(TEXT("non-poolable enemy battle subsystem is available"), BattleSubsystem);
			if (BattleSubsystem)
			{
				FText FailureReason;
				TestFalse(TEXT("battle start fails when enemy class is not poolable"), BattleSubsystem->StartDefaultBattle(FailureReason));
				TestTrue(TEXT("non-poolable enemy failure provides a reason"), HasFailureText(FailureReason));
				TestEqual(TEXT("non-poolable enemy failure leaves battle inactive"), BattleSubsystem->GetBattleState(), ETTDBattleState::Inactive);
			}

			DestroyBattleTestWorld(NonPoolableEnemyWorld);
		}
	}

	{
		ResetSettingsForBattleTest(Settings);
		FTTDBattleTestTables Tables;
		Tables.AddDefaultRows();
		Tables.Configure(Settings);

		const TStrongObjectPtr<UDataTable> ObjectPoolTable(CreateBattleObjectPoolDataTable());
		FTTDObjectPoolDefinition EnemyPoolDefinition;
		EnemyPoolDefinition.ObjectClass = ATTDBattleEnemyActor::StaticClass();
		EnemyPoolDefinition.InitialSize = 0;
		EnemyPoolDefinition.MaxSize = 0;
		EnemyPoolDefinition.ExpandBy = 1;
		EnemyPoolDefinition.bPrewarmOnWorldBeginPlay = false;
		ObjectPoolTable->AddRow(TEXT("EnemyPool"), EnemyPoolDefinition);
		ConfigureObjectPoolTable(Settings, ObjectPoolTable.Get());

		UWorld* SpawnFailureWorld = CreateBattleTestWorld();
		if (TestNotNull(TEXT("runtime spawn failure test world is created"), SpawnFailureWorld))
		{
			ATTDBattleSpawnPointActor* SpawnPoint = nullptr;
			ATTDBuildSlotActor* Slot = nullptr;
			SpawnRequiredBattleActors(SpawnFailureWorld, SpawnPoint, Slot);

			UTTDBattleWorldSubsystem* BattleSubsystem = SpawnFailureWorld->GetSubsystem<UTTDBattleWorldSubsystem>();
			TestNotNull(TEXT("runtime spawn failure battle subsystem is available"), BattleSubsystem);
			if (BattleSubsystem)
			{
				FText FailureReason;
				TestTrue(TEXT("battle can start before runtime pool capacity is exhausted"), BattleSubsystem->StartDefaultBattle(FailureReason));
				TestEqual(TEXT("runtime spawn failure starts with zero progress"), BattleSubsystem->GetProgress(), 0.0f);
				AddExpectedError(TEXT("Battle entered configuration error"), EAutomationExpectedErrorFlags::Contains, 1);
				BattleSubsystem->Tick(0.3f);
				TestEqual(TEXT("runtime enemy spawn failure enters configuration error"), BattleSubsystem->GetBattleState(), ETTDBattleState::ConfigurationError);
				TestEqual(TEXT("runtime enemy spawn failure does not advance progress"), BattleSubsystem->GetProgress(), 0.0f);
				TestTrue(TEXT("runtime enemy spawn failure does not trigger victory"), BattleSubsystem->GetBattleState() != ETTDBattleState::Victory);
			}

			DestroyBattleTestWorld(SpawnFailureWorld);
		}
	}

	{
		ResetSettingsForBattleTest(Settings);
		FTTDBattleTestTables Tables;
		Tables.AddDefaultRows();
		Tables.Configure(Settings);

		const TStrongObjectPtr<UDataTable> ObjectPoolTable(CreateBattleObjectPoolDataTable());
		FTTDObjectPoolDefinition ProjectilePoolDefinition;
		ProjectilePoolDefinition.ObjectClass = ATTDBattleProjectileActor::StaticClass();
		ProjectilePoolDefinition.InitialSize = 0;
		ProjectilePoolDefinition.MaxSize = 1;
		ProjectilePoolDefinition.ExpandBy = 1;
		ProjectilePoolDefinition.bPrewarmOnWorldBeginPlay = false;
		ObjectPoolTable->AddRow(TEXT("ProjectilePool"), ProjectilePoolDefinition);
		ConfigureObjectPoolTable(Settings, ObjectPoolTable.Get());

		UWorld* ProjectileLifecycleWorld = CreateBattleTestWorld();
		if (TestNotNull(TEXT("projectile lifecycle test world is created"), ProjectileLifecycleWorld))
		{
			ATTDBattleSpawnPointActor* SpawnPoint = nullptr;
			ATTDBuildSlotActor* Slot = nullptr;
			SpawnRequiredBattleActors(ProjectileLifecycleWorld, SpawnPoint, Slot);

			UTTDBattleWorldSubsystem* BattleSubsystem = ProjectileLifecycleWorld->GetSubsystem<UTTDBattleWorldSubsystem>();
			TestNotNull(TEXT("projectile lifecycle battle subsystem is available"), BattleSubsystem);
			if (BattleSubsystem && Slot)
			{
				FText FailureReason;
				TestTrue(TEXT("projectile lifecycle battle starts"), BattleSubsystem->StartDefaultBattle(FailureReason));
				TestTrue(TEXT("projectile tower can be built for lifecycle test"), BattleSubsystem->TryBuildOnSlot(TEXT("ProjectileTower"), Slot, FailureReason));

				ATTDBattleBuildingActor* ProjectileBuilding = Cast<ATTDBattleBuildingActor>(Slot->GetOccupyingBuilding());
				TestNotNull(TEXT("projectile lifecycle tower exists"), ProjectileBuilding);
				BattleSubsystem->Tick(0.31f);
				if (ProjectileBuilding)
				{
					ProjectileBuilding->Tick(0.02f);
				}

				ATTDBattleProjectileActor* Projectile = nullptr;
				for (TActorIterator<ATTDBattleProjectileActor> It(ProjectileLifecycleWorld); It; ++It)
				{
					if (It->IsActorTickEnabled())
					{
						Projectile = *It;
						break;
					}
				}

				TestEqual(TEXT("projectile is registered in battle subsystem after firing"), BattleSubsystem->GetActiveProjectileCount(), 1);
				TestNotNull(TEXT("registered projectile actor exists"), Projectile);
				BattleSubsystem->EndBattle(ETTDBattleState::Defeat);
				TestEqual(TEXT("ending battle releases active projectiles"), BattleSubsystem->GetActiveProjectileCount(), 0);
				TestTrue(TEXT("released projectile is no longer ticking"), !Projectile || !IsValid(Projectile) || !Projectile->IsActorTickEnabled() || Projectile->IsActorBeingDestroyed());
			}

			DestroyBattleTestWorld(ProjectileLifecycleWorld);
		}
	}

	{
		ResetSettingsForBattleTest(Settings);
		FTTDBattleTestTables Tables;
		Tables.AddDefaultRows();
		Tables.Configure(Settings);

		UWorld* LoadoutWorld = CreateBattleTestWorld();
		if (TestNotNull(TEXT("explicit loadout test world is created"), LoadoutWorld))
		{
			ATTDBattleSpawnPointActor* SpawnPoint = nullptr;
			ATTDBuildSlotActor* Slot = nullptr;
			SpawnRequiredBattleActors(LoadoutWorld, SpawnPoint, Slot);

			UTTDBattleWorldSubsystem* BattleSubsystem = LoadoutWorld->GetSubsystem<UTTDBattleWorldSubsystem>();
			TestNotNull(TEXT("explicit loadout battle subsystem is available"), BattleSubsystem);
			if (BattleSubsystem && Slot)
			{
				FTTDBattleLoadout EmptyLoadout;
				FText FailureReason;
				TestTrue(TEXT("battle starts with an explicit empty loadout"), BattleSubsystem->StartBattle(TEXT("TestLevel"), EmptyLoadout, FailureReason));
				const TArray<FTTDBuildingDefinition> EmptyLoadoutBuildings = BattleSubsystem->GetAvailableBuildingDefinitions();
				TestFalse(TEXT("empty loadout exposes no basic tower build option"), ContainsBuildingDefinition(EmptyLoadoutBuildings, TEXT("BasicTower")));
				TestFalse(TEXT("empty loadout exposes no projectile tower build option"), ContainsBuildingDefinition(EmptyLoadoutBuildings, TEXT("ProjectileTower")));
				TestFalse(TEXT("explicit empty loadout does not inherit level starting diagrams"), BattleSubsystem->TryBuildOnSlot(TEXT("BasicTower"), Slot, FailureReason));

				BattleSubsystem->EndBattle(ETTDBattleState::Defeat);
				BattleSubsystem->ClearEndedBattle();

				FTTDBattleLoadout SelectedLoadout;
				SelectedLoadout.SelectedDiagramIds = { TEXT("BasicTower") };
				SelectedLoadout.SelectedToyBoxes = { FTTDNameStack(TEXT("BasicToyBox"), 2) };
				TestTrue(TEXT("battle starts with a selected preparation loadout"), BattleSubsystem->StartBattle(TEXT("TestLevel"), SelectedLoadout, FailureReason));
				const TArray<FTTDBuildingDefinition> SelectedLoadoutBuildings = BattleSubsystem->GetAvailableBuildingDefinitions();
				TestTrue(TEXT("selected basic tower diagram exposes basic tower build option"), ContainsBuildingDefinition(SelectedLoadoutBuildings, TEXT("BasicTower")));
				TestFalse(TEXT("selected basic tower diagram hides projectile tower build option"), ContainsBuildingDefinition(SelectedLoadoutBuildings, TEXT("ProjectileTower")));
				TestFalse(TEXT("selected basic tower diagram does not enable projectile tower"), BattleSubsystem->TryBuildOnSlot(TEXT("ProjectileTower"), Slot, FailureReason));
				TestTrue(TEXT("selected diagram enables its building"), BattleSubsystem->TryBuildOnSlot(TEXT("BasicTower"), Slot, FailureReason));

				TArray<FTTDNameStack> Rewards;
				TestTrue(TEXT("first selected toy box is available in battle inventory"), BattleSubsystem->OpenToyBox(TEXT("BasicToyBox"), Rewards, FailureReason));
				TestTrue(TEXT("second selected toy box of same type is available in battle inventory"), BattleSubsystem->OpenToyBox(TEXT("BasicToyBox"), Rewards, FailureReason));
				TestFalse(TEXT("selected toy box stack is consumed after repeated opens"), BattleSubsystem->OpenToyBox(TEXT("BasicToyBox"), Rewards, FailureReason));
			}

			DestroyBattleTestWorld(LoadoutWorld);
		}
	}

	{
		ResetSettingsForBattleTest(Settings);
		FTTDBattleTestTables Tables;
		Tables.AddDefaultRows();
		if (FTTDBattleLevelDefinition* Level = Tables.LevelTable->FindRow<FTTDBattleLevelDefinition>(TEXT("TestLevel"), TEXT("BuildingCapacityShop")))
		{
			Level->StartingBuildingCapacity = 1;
			Level->BuildingCapacityPurchaseCost = 5;
		}
		if (FTTDBuildingDefinition* BasicTower = Tables.BuildingTable->FindRow<FTTDBuildingDefinition>(TEXT("BasicTower"), TEXT("BuildingCapacityShop")))
		{
			BasicTower->PartCosts.Reset();
		}
		Tables.Configure(Settings);

		UWorld* BuildingCapacityWorld = CreateBattleTestWorld();
		if (TestNotNull(TEXT("building capacity shop test world is created"), BuildingCapacityWorld))
		{
			ATTDBattleSpawnPointActor* SpawnPoint = nullptr;
			ATTDBuildSlotActor* Slot = nullptr;
			SpawnRequiredBattleActors(BuildingCapacityWorld, SpawnPoint, Slot);
			ATTDBuildSlotActor* ExtraSlot = SpawnSlot(BuildingCapacityWorld, TEXT("ExtraSlot"), FVector(0.0f, 220.0f, 0.0f));

			UTTDBattleWorldSubsystem* BattleSubsystem = BuildingCapacityWorld->GetSubsystem<UTTDBattleWorldSubsystem>();
			TestNotNull(TEXT("building capacity battle subsystem is available"), BattleSubsystem);
			if (BattleSubsystem && Slot && ExtraSlot)
			{
				FText FailureReason;
				TestTrue(TEXT("building capacity battle starts"), BattleSubsystem->StartDefaultBattle(FailureReason));
				TestEqual(TEXT("configured starting building capacity is exposed"), BattleSubsystem->GetBuildingCapacity(), 1);
				TestTrue(TEXT("first building fits capacity"), BattleSubsystem->TryBuildOnSlot(TEXT("BasicTower"), Slot, FailureReason));
				TestEqual(TEXT("building count tracks placed buildings"), BattleSubsystem->GetBuildingCount(), 1);
				TestFalse(TEXT("second building is blocked by capacity"), BattleSubsystem->TryBuildOnSlot(TEXT("BasicTower"), ExtraSlot, FailureReason));
				TestTrue(TEXT("building capacity can be purchased"), BattleSubsystem->BuyBuildingCapacity(FailureReason));
				TestEqual(TEXT("purchased building capacity increments cap"), BattleSubsystem->GetBuildingCapacity(), 2);
				TestTrue(TEXT("second building succeeds after capacity purchase"), BattleSubsystem->TryBuildOnSlot(TEXT("BasicTower"), ExtraSlot, FailureReason));
				TestEqual(TEXT("capacity purchase spends configured currency"), BattleSubsystem->GetCurrency(), 5);
			}

			DestroyBattleTestWorld(BuildingCapacityWorld);
		}
	}

	{
		ResetSettingsForBattleTest(Settings);
		FTTDBattleTestTables Tables;
		Tables.AddDefaultRows();
		if (FTTDBattleLevelDefinition* Level = Tables.LevelTable->FindRow<FTTDBattleLevelDefinition>(TEXT("TestLevel"), TEXT("BuildingUpgradeAndSell")))
		{
			Level->StartingBuildingLevelCap = 1;
			Level->BuildingLevelCapPurchaseCost = 4;
			Level->StartingCurrency = 10;
			Level->StartingParts = { FTTDNameStack(TEXT("Gear"), 5) };
		}
		Tables.Configure(Settings);

		UWorld* BuildingUpgradeWorld = CreateBattleTestWorld();
		if (TestNotNull(TEXT("building upgrade and sell test world is created"), BuildingUpgradeWorld))
		{
			ATTDBattleSpawnPointActor* SpawnPoint = nullptr;
			ATTDBuildSlotActor* Slot = nullptr;
			SpawnRequiredBattleActors(BuildingUpgradeWorld, SpawnPoint, Slot);

			UTTDBattleWorldSubsystem* BattleSubsystem = BuildingUpgradeWorld->GetSubsystem<UTTDBattleWorldSubsystem>();
			TestNotNull(TEXT("building upgrade battle subsystem is available"), BattleSubsystem);
			if (BattleSubsystem && Slot)
			{
				FText FailureReason;
				TestTrue(TEXT("building upgrade battle starts"), BattleSubsystem->StartDefaultBattle(FailureReason));
				TestEqual(TEXT("configured starting building level cap is exposed"), BattleSubsystem->GetBuildingLevelCap(), 1);
				TestTrue(TEXT("basic tower can be built for upgrade test"), BattleSubsystem->TryBuildOnSlot(TEXT("BasicTower"), Slot, FailureReason));

				ATTDBattleBuildingActor* Building = Cast<ATTDBattleBuildingActor>(Slot->GetOccupyingBuilding());
				TestNotNull(TEXT("upgrade test building exists"), Building);
				if (Building)
				{
					TestEqual(TEXT("new buildings start at level one"), Building->GetBuildingLevel(), 1);
					TestFalse(TEXT("level cap blocks upgrade before purchase"), BattleSubsystem->UpgradeBuilding(Building, TEXT("Gear"), FailureReason));
					TestTrue(TEXT("building level cap can be purchased"), BattleSubsystem->BuyBuildingLevelCap(FailureReason));
					TestEqual(TEXT("purchased building level cap increments cap"), BattleSubsystem->GetBuildingLevelCap(), 2);

					const int32 GearBeforeUpgrade = FindBattleTestStackCount(BattleSubsystem->GetPartStacks(), TEXT("Gear"));
					TestTrue(TEXT("configured part upgrades building"), BattleSubsystem->UpgradeBuilding(Building, TEXT("Gear"), FailureReason));
					TestEqual(TEXT("building level increments after upgrade"), Building->GetBuildingLevel(), 2);
					TestEqual(TEXT("upgrade modifier changes attack damage"), Building->GetRuntimeStats().AttackDamage, 8.0f);
					TestEqual(TEXT("upgrade consumes configured part count"), FindBattleTestStackCount(BattleSubsystem->GetPartStacks(), TEXT("Gear")), GearBeforeUpgrade - 1);
					TestFalse(TEXT("upgrade is blocked again at new cap"), BattleSubsystem->UpgradeBuilding(Building, TEXT("Gear"), FailureReason));

					const int32 GearBeforeSell = FindBattleTestStackCount(BattleSubsystem->GetPartStacks(), TEXT("Gear"));
					TestTrue(TEXT("building can be sold"), BattleSubsystem->SellBuilding(Building, FailureReason));
					TestNull(TEXT("sold building clears owning slot"), Slot->GetOccupyingBuilding());
					TestEqual(TEXT("sell refunds construction costs only"), FindBattleTestStackCount(BattleSubsystem->GetPartStacks(), TEXT("Gear")), GearBeforeSell + 2);
				}
			}

			DestroyBattleTestWorld(BuildingUpgradeWorld);
		}
	}

	{
		ResetSettingsForBattleTest(Settings);
		FTTDBattleTestTables Tables;
		Tables.AddDefaultRows();
		if (FTTDBattleLevelDefinition* Level = Tables.LevelTable->FindRow<FTTDBattleLevelDefinition>(TEXT("TestLevel"), TEXT("ArbitraryPlacement")))
		{
			Level->StartingParts = { FTTDNameStack(TEXT("Gear"), 8) };
			Level->bAllowArbitraryBuildPlacement = true;
			Level->ArbitraryBuildMinSpacing = 180.0f;
		}
		Tables.Configure(Settings);

		UWorld* ArbitraryPlacementWorld = CreateBattleTestWorld();
		if (TestNotNull(TEXT("arbitrary placement test world is created"), ArbitraryPlacementWorld))
		{
			ATTDBattleSpawnPointActor* SpawnPoint = nullptr;
			ATTDBuildSlotActor* Slot = nullptr;
			SpawnRequiredBattleActors(ArbitraryPlacementWorld, SpawnPoint, Slot);

			UTTDBattleWorldSubsystem* BattleSubsystem = ArbitraryPlacementWorld->GetSubsystem<UTTDBattleWorldSubsystem>();
			TestNotNull(TEXT("arbitrary placement battle subsystem is available"), BattleSubsystem);
			if (BattleSubsystem)
			{
				FText FailureReason;
				TestTrue(TEXT("arbitrary placement battle starts"), BattleSubsystem->StartDefaultBattle(FailureReason));
				TestTrue(TEXT("building can be placed at arbitrary ground location"), BattleSubsystem->TryBuildAtLocation(TEXT("BasicTower"), FVector(350.0f, 350.0f, 0.0f), FailureReason));
				TestEqual(TEXT("arbitrary placement creates one active building"), BattleSubsystem->GetBuildingCount(), 1);
				TestFalse(TEXT("arbitrary placement rejects locations too close to an existing building"), BattleSubsystem->TryBuildAtLocation(TEXT("BasicTower"), FVector(360.0f, 350.0f, 0.0f), FailureReason));
				TestTrue(TEXT("arbitrary placement allows a separated second building"), BattleSubsystem->TryBuildAtLocation(TEXT("BasicTower"), FVector(700.0f, 350.0f, 0.0f), FailureReason));
				TestEqual(TEXT("arbitrary placement tracks second building"), BattleSubsystem->GetBuildingCount(), 2);

				BattleSubsystem->EndBattle(ETTDBattleState::Defeat);
				BattleSubsystem->ClearEndedBattle();

				int32 RuntimeSlotCount = 0;
				for (TActorIterator<ATTDBuildSlotActor> It(ArbitraryPlacementWorld); It; ++It)
				{
					if (It->IsRuntimeGeneratedSlot() && !It->IsActorBeingDestroyed())
					{
						++RuntimeSlotCount;
					}
				}
				TestEqual(TEXT("clearing battle removes runtime arbitrary build slots"), RuntimeSlotCount, 0);
			}

			DestroyBattleTestWorld(ArbitraryPlacementWorld);
		}
	}

	{
		ResetSettingsForBattleTest(Settings);
		FTTDBattleTestTables Tables;
		Tables.AddDefaultRows();
		if (FTTDBattleLevelDefinition* Level = Tables.LevelTable->FindRow<FTTDBattleLevelDefinition>(TEXT("TestLevel"), TEXT("LegacyToyBoxId")))
		{
			Level->StartingToyBoxes = { FTTDNameStack(TEXT("BasicBox"), 1) };
		}
		if (FTTDToyBoxRewardDefinition* ToyBox = Tables.ToyBoxRewardTable->FindRow<FTTDToyBoxRewardDefinition>(TEXT("BasicToyBox"), TEXT("LegacyToyBoxId")))
		{
			FTTDToyBoxRewardDefinition LegacyToyBox = *ToyBox;
			Tables.ToyBoxRewardTable->RemoveRow(TEXT("BasicToyBox"));
			LegacyToyBox.ToyBoxId = TEXT("BasicBox");
			Tables.ToyBoxRewardTable->AddRow(TEXT("BasicBox"), LegacyToyBox);
		}
		Tables.Configure(Settings);

		UWorld* LegacyToyBoxWorld = CreateBattleTestWorld();
		if (TestNotNull(TEXT("legacy toy box id test world is created"), LegacyToyBoxWorld))
		{
			ATTDBattleSpawnPointActor* SpawnPoint = nullptr;
			ATTDBuildSlotActor* Slot = nullptr;
			SpawnRequiredBattleActors(LegacyToyBoxWorld, SpawnPoint, Slot);

			UTTDBattleWorldSubsystem* BattleSubsystem = LegacyToyBoxWorld->GetSubsystem<UTTDBattleWorldSubsystem>();
			TestNotNull(TEXT("legacy toy box id battle subsystem is available"), BattleSubsystem);
			if (BattleSubsystem)
			{
				TestNotNull(TEXT("legacy BasicBox reward definition is exposed as BasicToyBox"), BattleSubsystem->FindToyBoxDefinition(TEXT("BasicToyBox")));
				FText FailureReason;
				TestTrue(TEXT("legacy BasicBox starting stack is normalized for battle"), BattleSubsystem->StartDefaultBattle(FailureReason));

				TArray<FTTDNameStack> Rewards;
				TestTrue(TEXT("normalized BasicToyBox can open legacy BasicBox rewards"), BattleSubsystem->OpenToyBox(TEXT("BasicToyBox"), Rewards, FailureReason));
			}

			DestroyBattleTestWorld(LegacyToyBoxWorld);
		}
	}

	{
		ResetSettingsForBattleTest(Settings);
		FTTDBattleTestTables Tables;
		Tables.AddDefaultRows();
		Tables.Configure(Settings);

		UWorld* CheatWorld = CreateBattleTestWorld();
		if (TestNotNull(TEXT("cheat supplies test world is created"), CheatWorld))
		{
			ATTDBattleSpawnPointActor* SpawnPoint = nullptr;
			ATTDBuildSlotActor* Slot = nullptr;
			SpawnRequiredBattleActors(CheatWorld, SpawnPoint, Slot);

			UTTDBattleWorldSubsystem* BattleSubsystem = CheatWorld->GetSubsystem<UTTDBattleWorldSubsystem>();
			TestNotNull(TEXT("cheat supplies battle subsystem is available"), BattleSubsystem);
			if (BattleSubsystem && Slot)
			{
				FTTDBattleLoadout EmptyLoadout;
				FText FailureReason;
				TestTrue(TEXT("cheat supplies battle starts with empty loadout"), BattleSubsystem->StartBattle(TEXT("TestLevel"), EmptyLoadout, FailureReason));
				TestFalse(TEXT("projectile tower is initially blocked without cheat resources"), BattleSubsystem->TryBuildOnSlot(TEXT("ProjectileTower"), Slot, FailureReason));

				FText CheatSummary;
				TestTrue(TEXT("cheat supplies can be applied while battle is running"), BattleSubsystem->ApplyTestCheatSupplies(CheatSummary));
				TestTrue(TEXT("cheat supplies grant high test currency"), BattleSubsystem->GetCurrency() >= 999);
				TestTrue(
					TEXT("cheat supplies expose projectile tower build option after granting diagrams"),
					ContainsBuildingDefinition(BattleSubsystem->GetAvailableBuildingDefinitions(), TEXT("ProjectileTower")));
				TestTrue(TEXT("cheat supplies unlock missing diagram and parts"), BattleSubsystem->TryBuildOnSlot(TEXT("ProjectileTower"), Slot, FailureReason));

				TArray<FTTDNameStack> Rewards;
				TestTrue(TEXT("cheat supplies grant configured toy boxes"), BattleSubsystem->OpenToyBox(TEXT("BasicToyBox"), Rewards, FailureReason));
			}

			DestroyBattleTestWorld(CheatWorld);
		}
	}

	{
		ResetSettingsForBattleTest(Settings);
		FTTDBattleTestTables Tables;
		Tables.AddDefaultRows();
		if (FTTDEnemyDefinition* Enemy = Tables.EnemyTable->FindRow<FTTDEnemyDefinition>(TEXT("BasicEnemy"), TEXT("NoCurrencyDropChance")))
		{
			Enemy->CurrencyDrop = 7;
			Enemy->CurrencyDropChance = 0.0f;
		}
		Tables.Configure(Settings);

		UWorld* NoDropWorld = CreateBattleTestWorld();
		if (TestNotNull(TEXT("no currency drop chance test world is created"), NoDropWorld))
		{
			ATTDBattleSpawnPointActor* SpawnPoint = nullptr;
			ATTDBuildSlotActor* Slot = nullptr;
			SpawnRequiredBattleActors(NoDropWorld, SpawnPoint, Slot);

			UTTDBattleWorldSubsystem* BattleSubsystem = NoDropWorld->GetSubsystem<UTTDBattleWorldSubsystem>();
			TestNotNull(TEXT("no currency drop chance battle subsystem is available"), BattleSubsystem);
			if (BattleSubsystem)
			{
				FText FailureReason;
				TestTrue(TEXT("no drop chance battle starts"), BattleSubsystem->StartDefaultBattle(FailureReason));
				BattleSubsystem->Tick(0.3f);
				ATTDBattleEnemyActor* Enemy = BattleSubsystem->FindNearestEnemy(FVector::ZeroVector, 10000.0f);
				TestNotNull(TEXT("no drop chance enemy spawns"), Enemy);
				const int32 CurrencyBeforeKill = BattleSubsystem->GetCurrency();
				if (Enemy)
				{
					Enemy->ApplyDamage(999.0f, BattleSubsystem);
				}
				TestEqual(TEXT("zero currency drop chance grants no currency"), BattleSubsystem->GetCurrency(), CurrencyBeforeKill);
			}

			DestroyBattleTestWorld(NoDropWorld);
		}
	}

	{
		ResetSettingsForBattleTest(Settings);
		FTTDBattleTestTables Tables;
		Tables.AddDefaultRows();
		if (FTTDEnemyDefinition* Enemy = Tables.EnemyTable->FindRow<FTTDEnemyDefinition>(TEXT("BasicEnemy"), TEXT("GuaranteedCurrencyDropChance")))
		{
			Enemy->CurrencyDrop = 7;
			Enemy->CurrencyDropChance = 1.0f;
		}
		Tables.Configure(Settings);

		UWorld* GuaranteedDropWorld = CreateBattleTestWorld();
		if (TestNotNull(TEXT("guaranteed currency drop chance test world is created"), GuaranteedDropWorld))
		{
			ATTDBattleSpawnPointActor* SpawnPoint = nullptr;
			ATTDBuildSlotActor* Slot = nullptr;
			SpawnRequiredBattleActors(GuaranteedDropWorld, SpawnPoint, Slot);

			UTTDBattleWorldSubsystem* BattleSubsystem = GuaranteedDropWorld->GetSubsystem<UTTDBattleWorldSubsystem>();
			TestNotNull(TEXT("guaranteed currency drop chance battle subsystem is available"), BattleSubsystem);
			if (BattleSubsystem)
			{
				FText FailureReason;
				TestTrue(TEXT("guaranteed drop chance battle starts"), BattleSubsystem->StartDefaultBattle(FailureReason));
				BattleSubsystem->Tick(0.3f);
				ATTDBattleEnemyActor* Enemy = BattleSubsystem->FindNearestEnemy(FVector::ZeroVector, 10000.0f);
				TestNotNull(TEXT("guaranteed drop chance enemy spawns"), Enemy);
				const int32 CurrencyBeforeKill = BattleSubsystem->GetCurrency();
				if (Enemy)
				{
					Enemy->ApplyDamage(999.0f, BattleSubsystem);
				}
				TestEqual(TEXT("one currency drop chance grants configured currency"), BattleSubsystem->GetCurrency(), CurrencyBeforeKill + 7);
			}

			DestroyBattleTestWorld(GuaranteedDropWorld);
		}
	}

	ResetSettingsForBattleTest(Settings);
	FTTDBattleTestTables HappyTables;
	HappyTables.AddDefaultRows();
	HappyTables.Configure(Settings);

	UWorld* World = CreateBattleTestWorld();
	if (!TestNotNull(TEXT("test world is created"), World))
	{
		RestoreSettings();
		return false;
	}

	ATTDBattleSpawnPointActor* SpawnPoint = World->SpawnActor<ATTDBattleSpawnPointActor>(FVector(300.0f, 0.0f, 0.0f), FRotator::ZeroRotator);
	TestNotNull(TEXT("spawn point is created"), SpawnPoint);
	if (SpawnPoint)
	{
		SpawnPoint->ConfigureSpawnPoint(TEXT("Edge"));
	}

	ATTDBuildSlotActor* HighSlot = SpawnSlot(World, TEXT("HighSlot"), FVector(0.0f, 100.0f, 0.0f), TEXT("HighGround"));
	ATTDBuildSlotActor* PlainSlot = SpawnSlot(World, TEXT("PlainSlot"), FVector(0.0f, 220.0f, 0.0f));
	ATTDBuildSlotActor* ProjectileSlot = SpawnSlot(World, TEXT("ProjectileSlot"), FVector(0.0f, 340.0f, 0.0f));
	TestNotNull(TEXT("high slot is created"), HighSlot);
	TestNotNull(TEXT("plain slot is created"), PlainSlot);
	TestNotNull(TEXT("projectile slot is created"), ProjectileSlot);

	UTTDBattleWorldSubsystem* BattleSubsystem = World->GetSubsystem<UTTDBattleWorldSubsystem>();
	if (!TestNotNull(TEXT("battle subsystem is available"), BattleSubsystem))
	{
		DestroyBattleTestWorld(World);
		RestoreSettings();
		return false;
	}

	TestTrue(TEXT("battle starts from configured default level"), BattleSubsystem->StartDefaultBattle());
	ATTDBattleBackgroundActor* BackgroundActor = nullptr;
	for (TActorIterator<ATTDBattleBackgroundActor> It(World); It; ++It)
	{
		BackgroundActor = *It;
		break;
	}
	TestNotNull(TEXT("battle start spawns a background actor"), BackgroundActor);
	if (BackgroundActor)
	{
		TestEqual(TEXT("battle background uses configured X half extent"), BackgroundActor->GetBackgroundDefinition().ArenaHalfExtent.X, 1200.0);
		TestEqual(TEXT("battle background uses configured Y half extent"), BackgroundActor->GetBackgroundDefinition().ArenaHalfExtent.Y, 900.0);
	}
	TestEqual(TEXT("battle starts in preparation phase"), BattleSubsystem->GetBattlePhase(), ETTDBattlePhase::Preparing);
	TestEqual(TEXT("current wave is first wave during preparation"), BattleSubsystem->GetCurrentWaveNumber(), 1);
	TestEqual(TEXT("total wave count is exposed"), BattleSubsystem->GetTotalWaveCount(), 1);
	TestEqual(TEXT("total level weight is built from wave enemy weights"), BattleSubsystem->GetTotalEnemyWeight(), 4.0f);
	TestEqual(TEXT("initial remaining weight includes unspawned enemies"), BattleSubsystem->GetRemainingEnemyWeight(), 4.0f);
	TestEqual(TEXT("initial progress is zero"), BattleSubsystem->GetProgress(), 0.0f);

	FText FailureReason;
	TestFalse(TEXT("building without required diagram is rejected"), BattleSubsystem->TryBuildOnSlot(TEXT("MissingDiagramTower"), HighSlot, FailureReason));
	TestFalse(TEXT("building with insufficient parts is rejected"), BattleSubsystem->TryBuildOnSlot(TEXT("ExpensiveTower"), HighSlot, FailureReason));
	TestTrue(TEXT("height does not block successful building placement"), BattleSubsystem->TryBuildOnSlot(TEXT("BasicTower"), HighSlot, FailureReason));
	TestTrue(TEXT("occupied slot is rejected"), !BattleSubsystem->TryBuildOnSlot(TEXT("BasicTower"), HighSlot, FailureReason));

	ATTDBattleBuildingActor* HighBuilding = Cast<ATTDBattleBuildingActor>(HighSlot->GetOccupyingBuilding());
	TestNotNull(TEXT("successful build creates a battle building"), HighBuilding);
	if (HighBuilding)
	{
		const FTTDBattleBuildingRuntimeStats Stats = HighBuilding->GetRuntimeStats();
		TestEqual(TEXT("height add modifier affects attack damage"), Stats.AttackDamage, 10.0f);
		TestEqual(TEXT("height multiply modifier affects range"), Stats.AttackRange, 2000.0f);
		TestEqual(TEXT("height override modifier affects interval"), Stats.AttackInterval, 0.25f);
		TestEqual(TEXT("height override modifier affects move speed placeholder"), Stats.MoveSpeed, 12.0f);
	}

	TestTrue(TEXT("building without height effect keeps base stats"), BattleSubsystem->TryBuildOnSlot(TEXT("BasicTower"), PlainSlot, FailureReason));
	ATTDBattleBuildingActor* PlainBuilding = Cast<ATTDBattleBuildingActor>(PlainSlot->GetOccupyingBuilding());
	TestNotNull(TEXT("plain slot build creates a building"), PlainBuilding);
	if (PlainBuilding)
	{
		const FTTDBattleBuildingRuntimeStats Stats = PlainBuilding->GetRuntimeStats();
		TestEqual(TEXT("base attack damage is unchanged without height effect"), Stats.AttackDamage, 5.0f);
		TestEqual(TEXT("base attack range is unchanged without height effect"), Stats.AttackRange, 1000.0f);
		TestEqual(TEXT("base attack interval is unchanged without height effect"), Stats.AttackInterval, 0.5f);
		TestEqual(TEXT("base move speed defaults to stationary building"), Stats.MoveSpeed, 0.0f);
	}

	UObject* NearestTarget = BattleSubsystem->FindNearestBattleTarget(PlainSlot->GetActorLocation() + FVector(1.0f, 0.0f, 0.0f));
	if (HighBuilding && PlainBuilding)
	{
		HighBuilding->SetActorLocation(FVector::ZeroVector);
		PlainBuilding->SetActorLocation(FVector(1000.0f, 0.0f, 0.0f));
		NearestTarget = BattleSubsystem->FindNearestBattleTarget(FVector(999.0f, 0.0f, 0.0f));
	}
	TestTrue(TEXT("nearest target selection prefers closest building over castle"), NearestTarget == PlainBuilding);

	TestTrue(TEXT("toy box can be purchased with currency"), BattleSubsystem->BuyToyBox(TEXT("BasicToyBox"), FailureReason));
	BattleSubsystem->SetRewardRandomSeed(42);
	TArray<FTTDNameStack> Rewards;
	TestTrue(TEXT("toy box opens through weighted reward table"), BattleSubsystem->OpenToyBox(TEXT("BasicToyBox"), Rewards, FailureReason));
	TestEqual(TEXT("toy box deterministic reward stack count"), Rewards.Num(), 1);
	if (Rewards.Num() == 1)
	{
		TestEqual(TEXT("toy box rewards configured part"), Rewards[0].Id, FName(TEXT("Gear")));
		TestEqual(TEXT("toy box rewards configured quantity"), Rewards[0].Count, 2);
	}

	BattleSubsystem->Tick(0.1f);
	TestNull(TEXT("enemy does not spawn during preparation"), BattleSubsystem->FindNearestEnemy(FVector::ZeroVector, 10000.0f));
	BattleSubsystem->Tick(0.1f);
	TestEqual(TEXT("battle enters first buffer before wave"), BattleSubsystem->GetBattlePhase(), ETTDBattlePhase::Buffer);
	BattleSubsystem->Tick(0.1f);
	TestEqual(TEXT("battle enters wave active after preparation and buffer"), BattleSubsystem->GetBattlePhase(), ETTDBattlePhase::WaveActive);
	ATTDBattleEnemyActor* FirstEnemy = BattleSubsystem->FindNearestEnemy(FVector::ZeroVector, 10000.0f);
	TestNotNull(TEXT("first enemy spawns from wave schedule"), FirstEnemy);
	if (HighBuilding)
	{
		HighBuilding->Tick(0.2f);
	}
	TestEqual(TEXT("instant damage building kills first enemy and advances progress"), BattleSubsystem->GetProgress(), 0.5f);
	TestEqual(TEXT("enemy death grants currency"), BattleSubsystem->GetCurrency(), 8);

	TestTrue(TEXT("projectile tower can be built after toy box rewards"), BattleSubsystem->TryBuildOnSlot(TEXT("ProjectileTower"), ProjectileSlot, FailureReason));
	ATTDBattleBuildingActor* ProjectileBuilding = Cast<ATTDBattleBuildingActor>(ProjectileSlot->GetOccupyingBuilding());
	TestNotNull(TEXT("projectile tower build creates a building"), ProjectileBuilding);

	BattleSubsystem->Tick(0.2f);
	ATTDBattleEnemyActor* SecondEnemy = BattleSubsystem->FindNearestEnemy(FVector::ZeroVector, 10000.0f);
	TestNotNull(TEXT("second enemy spawns from wave schedule"), SecondEnemy);
	if (ProjectileBuilding)
	{
		ProjectileBuilding->Tick(0.02f);
	}
	TestEqual(TEXT("projectile mode registers active projectile"), BattleSubsystem->GetActiveProjectileCount(), 1);

	ATTDBattleProjectileActor* Projectile = nullptr;
	for (TActorIterator<ATTDBattleProjectileActor> It(World); It; ++It)
	{
		if (It->IsActorTickEnabled())
		{
			Projectile = *It;
			break;
		}
	}
	TestNotNull(TEXT("projectile mode spawns a pooled or temporary projectile"), Projectile);
	if (Projectile)
	{
		for (int32 TickIndex = 0; TickIndex < 20 && SecondEnemy && SecondEnemy->IsAlive(); ++TickIndex)
		{
			Projectile->Tick(0.01f);
		}
	}

	TestEqual(TEXT("final enemy death before wave timer expires keeps battle running"), BattleSubsystem->GetBattleState(), ETTDBattleState::Running);
	BattleSubsystem->Tick(0.3f);
	TestEqual(TEXT("projectile hit kills final enemy after wave timer expires"), BattleSubsystem->GetBattleState(), ETTDBattleState::Victory);
	TestEqual(TEXT("victory progress is complete"), BattleSubsystem->GetProgress(), 1.0f);
	TestEqual(TEXT("projectile is unregistered after battle ends"), BattleSubsystem->GetActiveProjectileCount(), 0);
	TestFalse(TEXT("victory return is not immediate"), BattleSubsystem->IsPostBattleReturnReady());
	BattleSubsystem->Tick(0.5f);
	TestTrue(TEXT("victory return becomes ready after configured delay"), BattleSubsystem->IsPostBattleReturnReady());
	BattleSubsystem->ClearEndedBattle();

	int32 BackgroundCountAfterCleanup = 0;
	for (TActorIterator<ATTDBattleBackgroundActor> It(World); It; ++It)
	{
		if (IsValid(*It) && !It->IsActorBeingDestroyed())
		{
			++BackgroundCountAfterCleanup;
		}
	}
	TestEqual(TEXT("clearing ended battle removes the spawned background"), BackgroundCountAfterCleanup, 0);

	DestroyBattleTestWorld(World);
	RestoreSettings();
	return true;
}

#endif
