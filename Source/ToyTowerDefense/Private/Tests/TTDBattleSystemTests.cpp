#include "Misc/AutomationTest.h"

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
	UDataTable* CreateDataTable(UScriptStruct* RowStruct)
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
		Level.StartingDiagramIds = { TEXT("BasicTower") };
		Level.StartingParts = { FTTDNameStack(TEXT("Gear"), 5), FTTDNameStack(TEXT("Spring"), 1) };
		Level.StartingToyBoxes = { FTTDNameStack(TEXT("BasicBox"), 1) };
		Level.StartingCurrency = 10;
		LevelTable->AddRow(TEXT("TestLevel"), Level);

		FTTDWaveDefinition Wave;
		Wave.WaveId = TEXT("WaveA");
		FTTDWaveEnemyEntry EnemyEntry;
		EnemyEntry.EnemyId = TEXT("BasicEnemy");
		EnemyEntry.Count = 2;
		EnemyEntry.SpawnInterval = 0.1f;
		EnemyEntry.SpawnGroup = TEXT("Edge");
		Wave.EnemyEntries = { EnemyEntry };
		WaveTable->AddRow(TEXT("WaveA"), Wave);

		FTTDEnemyDefinition Enemy;
		Enemy.EnemyId = TEXT("BasicEnemy");
		Enemy.EnemyClass = ATTDBattleEnemyActor::StaticClass();
		Enemy.MaxHealth = 10.0f;
		Enemy.MoveSpeed = 0.0f;
		Enemy.AttackDamage = 0.0f;
		Enemy.ProgressWeight = 2.0f;
		Enemy.CurrencyDrop = 3;
		EnemyTable->AddRow(TEXT("BasicEnemy"), Enemy);

		FTTDBuildingDefinition BasicTower;
		BasicTower.BuildingId = TEXT("BasicTower");
		BasicTower.RequiredDiagramId = TEXT("BasicTower");
		BasicTower.BuildingClass = ATTDBattleBuildingActor::StaticClass();
		BasicTower.PartCosts = { FTTDNameStack(TEXT("Gear"), 2) };
		BasicTower.AttackMode = ETTDBuildingAttackMode::InstantDamage;
		BasicTower.BaseStats.AttackDamage = 5.0f;
		BasicTower.BaseStats.AttackRange = 1000.0f;
		BasicTower.BaseStats.AttackInterval = 0.5f;
		BasicTower.BaseStats.MaxHealth = 50.0f;
		BuildingTable->AddRow(TEXT("BasicTower"), BasicTower);

		FTTDBuildingDefinition MissingDiagramTower = BasicTower;
		MissingDiagramTower.BuildingId = TEXT("MissingDiagramTower");
		MissingDiagramTower.RequiredDiagramId = TEXT("MissingDiagram");
		BuildingTable->AddRow(TEXT("MissingDiagramTower"), MissingDiagramTower);

		FTTDBuildingDefinition ExpensiveTower = BasicTower;
		ExpensiveTower.BuildingId = TEXT("ExpensiveTower");
		ExpensiveTower.PartCosts = { FTTDNameStack(TEXT("Gear"), 999) };
		BuildingTable->AddRow(TEXT("ExpensiveTower"), ExpensiveTower);

		FTTDBuildingDefinition ProjectileTower = BasicTower;
		ProjectileTower.BuildingId = TEXT("ProjectileTower");
		ProjectileTower.PartCosts = { FTTDNameStack(TEXT("Gear"), 1) };
		ProjectileTower.AttackMode = ETTDBuildingAttackMode::Projectile;
		ProjectileTower.BaseStats.AttackDamage = 10.0f;
		ProjectileTower.BaseStats.AttackRange = 1000.0f;
		ProjectileTower.BaseStats.AttackInterval = 0.01f;
		ProjectileTower.BaseStats.ProjectileSpeed = 3000.0f;
		ProjectileTower.ProjectileClass = ATTDBattleProjectileActor::StaticClass();
		BuildingTable->AddRow(TEXT("ProjectileTower"), ProjectileTower);

		FTTDToyBoxRewardDefinition ToyBox;
		ToyBox.ToyBoxId = TEXT("BasicBox");
		ToyBox.PurchaseCost = 5;
		ToyBox.RollCount = 2;
		FTTDToyBoxRewardEntry Reward;
		Reward.PartId = TEXT("Gear");
		Reward.Weight = 1.0f;
		Reward.MinQuantity = 1;
		Reward.MaxQuantity = 1;
		ToyBox.RewardEntries = { Reward };
		ToyBoxRewardTable->AddRow(TEXT("BasicBox"), ToyBox);

		FTTDBattleHeightEffectDefinition HeightEffect;
		HeightEffect.HeightEffectId = TEXT("HighGround");
		HeightEffect.Modifiers = {
			{ ETTDBattleAttribute::AttackDamage, ETTDAttributeModifierOp::Add, 5.0f },
			{ ETTDBattleAttribute::AttackRange, ETTDAttributeModifierOp::Multiply, 2.0f },
			{ ETTDBattleAttribute::AttackInterval, ETTDAttributeModifierOp::Override, 0.25f }
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
		return CreateDataTable(FTTDObjectPoolDefinition::StaticStruct());
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
			: LevelTable(CreateDataTable(FTTDBattleLevelDefinition::StaticStruct()))
			, WaveTable(CreateDataTable(FTTDWaveDefinition::StaticStruct()))
			, EnemyTable(CreateDataTable(FTTDEnemyDefinition::StaticStruct()))
			, BuildingTable(CreateDataTable(FTTDBuildingDefinition::StaticStruct()))
			, ToyBoxRewardTable(CreateDataTable(FTTDToyBoxRewardDefinition::StaticStruct()))
			, HeightEffectTable(CreateDataTable(FTTDBattleHeightEffectDefinition::StaticStruct()))
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

	auto RestoreSettings = [Settings, OriginalDataTables, OriginalDefaultBattleLevelId]()
	{
		Settings->GameplayTagDataTables = OriginalDataTables;
		Settings->DefaultBattleLevelId = OriginalDefaultBattleLevelId;
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
				BattleSubsystem->Tick(0.01f);
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
				BattleSubsystem->Tick(0.01f);
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
	}

	UObject* NearestTarget = BattleSubsystem->FindNearestBattleTarget(PlainSlot->GetActorLocation() + FVector(1.0f, 0.0f, 0.0f));
	if (HighBuilding && PlainBuilding)
	{
		HighBuilding->SetActorLocation(FVector::ZeroVector);
		PlainBuilding->SetActorLocation(FVector(1000.0f, 0.0f, 0.0f));
		NearestTarget = BattleSubsystem->FindNearestBattleTarget(FVector(999.0f, 0.0f, 0.0f));
	}
	TestTrue(TEXT("nearest target selection prefers closest building over castle"), NearestTarget == PlainBuilding);

	TestTrue(TEXT("toy box can be purchased with currency"), BattleSubsystem->BuyToyBox(TEXT("BasicBox"), FailureReason));
	BattleSubsystem->SetRewardRandomSeed(42);
	TArray<FTTDNameStack> Rewards;
	TestTrue(TEXT("toy box opens through weighted reward table"), BattleSubsystem->OpenToyBox(TEXT("BasicBox"), Rewards, FailureReason));
	TestEqual(TEXT("toy box deterministic reward stack count"), Rewards.Num(), 1);
	if (Rewards.Num() == 1)
	{
		TestEqual(TEXT("toy box rewards configured part"), Rewards[0].Id, FName(TEXT("Gear")));
		TestEqual(TEXT("toy box rewards configured quantity"), Rewards[0].Count, 2);
	}

	BattleSubsystem->Tick(0.01f);
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

	TestEqual(TEXT("projectile hit kills final enemy"), BattleSubsystem->GetBattleState(), ETTDBattleState::Victory);
	TestEqual(TEXT("victory progress is complete"), BattleSubsystem->GetProgress(), 1.0f);
	TestEqual(TEXT("projectile is unregistered after battle ends"), BattleSubsystem->GetActiveProjectileCount(), 0);

	DestroyBattleTestWorld(World);
	RestoreSettings();
	return true;
}

#endif
