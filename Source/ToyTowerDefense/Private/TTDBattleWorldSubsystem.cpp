#include "TTDBattleWorldSubsystem.h"

#include "Battle/TTDBattleBuildingActor.h"
#include "Battle/TTDBattleCastleActor.h"
#include "Battle/TTDBattleEnemyActor.h"
#include "Battle/TTDBattleProjectileActor.h"
#include "Battle/TTDBattleSpawnPointActor.h"
#include "Battle/TTDBattleTargetInterface.h"
#include "Battle/TTDBuildSlotActor.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "TTDDeveloperSettings.h"
#include "TTDGameplayMessages.h"
#include "TTDGameplayTags.h"
#include "TTDObjectPoolWorldSubsystem.h"
#include "TTDPooledObjectInterface.h"

DEFINE_LOG_CATEGORY_STATIC(LogTTDBattle, Log, All);

namespace
{
	FText BattleText(const TCHAR* Value)
	{
		return FText::FromString(Value);
	}

	template<typename RowType>
	bool IsCompatibleTable(const UDataTable* DataTable)
	{
		return DataTable && DataTable->GetRowStruct() == RowType::StaticStruct();
	}

	template<typename RowType, typename IdSetterType>
	void LoadRowsIntoMap(const UDataTable* DataTable, TMap<FName, RowType>& OutRows, IdSetterType SetId)
	{
		if (!IsCompatibleTable<RowType>(DataTable))
		{
			return;
		}

		for (const TPair<FName, uint8*>& RowPair : DataTable->GetRowMap())
		{
			const RowType* SourceRow = reinterpret_cast<const RowType*>(RowPair.Value);
			if (!SourceRow)
			{
				continue;
			}

			RowType Row = *SourceRow;
			SetId(Row, RowPair.Key);
			OutRows.Add(RowPair.Key, Row);
		}
	}

	TArray<FTTDNameStack> StackArrayFromMap(const TMap<FName, int32>& Source)
	{
		TArray<FTTDNameStack> Result;
		Result.Reserve(Source.Num());
		for (const TPair<FName, int32>& Pair : Source)
		{
			if (!Pair.Key.IsNone() && Pair.Value > 0)
			{
				Result.Add(FTTDNameStack(Pair.Key, Pair.Value));
			}
		}
		return Result;
	}

	void ApplyModifier(float& Value, const FTTDBattleAttributeModifier& Modifier)
	{
		switch (Modifier.Operation)
		{
		case ETTDAttributeModifierOp::Add:
			Value += Modifier.Value;
			break;
		case ETTDAttributeModifierOp::Multiply:
			Value *= Modifier.Value;
			break;
		case ETTDAttributeModifierOp::Override:
			Value = Modifier.Value;
			break;
		default:
			break;
		}
	}

	bool IsBattleTargetAlive(UObject* Target)
	{
		if (const ITTDBattleTargetInterface* NativeTarget = Cast<ITTDBattleTargetInterface>(Target))
		{
			return NativeTarget->IsBattleTargetAlive_Implementation();
		}

		return Target && Target->GetClass()->ImplementsInterface(UTTDBattleTargetInterface::StaticClass())
			? ITTDBattleTargetInterface::Execute_IsBattleTargetAlive(Target)
			: false;
	}

	FVector GetBattleTargetLocation(UObject* Target)
	{
		if (const ITTDBattleTargetInterface* NativeTarget = Cast<ITTDBattleTargetInterface>(Target))
		{
			return NativeTarget->GetBattleTargetLocation_Implementation();
		}

		return Target && Target->GetClass()->ImplementsInterface(UTTDBattleTargetInterface::StaticClass())
			? ITTDBattleTargetInterface::Execute_GetBattleTargetLocation(Target)
			: FVector::ZeroVector;
	}
}

void UTTDBattleWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	RewardRandomStream.Initialize(1337);
	LoadDefinitionsFromSettings();
}

void UTTDBattleWorldSubsystem::Deinitialize()
{
	CleanupBattle();
	Super::Deinitialize();
}

void UTTDBattleWorldSubsystem::Tick(const float DeltaTime)
{
	if (BattleState != ETTDBattleState::Running || DeltaTime <= 0.0f)
	{
		return;
	}

	BattleElapsedSeconds += DeltaTime;
	SpawnDueEnemies();
	CheckVictoryCondition();
}

TStatId UTTDBattleWorldSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UTTDBattleWorldSubsystem, STATGROUP_Tickables);
}

bool UTTDBattleWorldSubsystem::IsTickable() const
{
	return !IsTemplate() && !HasAnyFlags(RF_ClassDefaultObject);
}

UWorld* UTTDBattleWorldSubsystem::GetTickableGameObjectWorld() const
{
	return GetWorld();
}

bool UTTDBattleWorldSubsystem::StartDefaultBattle()
{
	FText FailureReason;
	return StartDefaultBattle(FailureReason);
}

bool UTTDBattleWorldSubsystem::StartDefaultBattle(FText& OutFailureReason)
{
	const UTTDDeveloperSettings* Settings = GetDefault<UTTDDeveloperSettings>();
	return StartBattle(Settings ? Settings->DefaultBattleLevelId : NAME_None, OutFailureReason);
}

bool UTTDBattleWorldSubsystem::StartDefaultBattleWithReason(FText& OutFailureReason)
{
	return StartDefaultBattle(OutFailureReason);
}

bool UTTDBattleWorldSubsystem::StartBattle(const FName LevelId)
{
	FText FailureReason;
	return StartBattle(LevelId, FailureReason);
}

bool UTTDBattleWorldSubsystem::StartBattleWithReason(const FName LevelId, FText& OutFailureReason)
{
	return StartBattle(LevelId, OutFailureReason);
}

bool UTTDBattleWorldSubsystem::StartBattle(const FName LevelId, FText& OutFailureReason)
{
	LoadDefinitionsFromSettings();
	GatherPlacedActors();

	const FTTDBattleLevelDefinition* LevelDefinition = LevelDefinitions.Find(LevelId);
	if (!LevelDefinition)
	{
		OutFailureReason = FText::Format(BattleText(TEXT("Battle level {0} is not configured.")), FText::FromName(LevelId));
		UE_LOG(LogTTDBattle, Warning, TEXT("StartBattle failed: %s"), *OutFailureReason.ToString());
		return false;
	}

	if (!ValidateBattleStart(LevelId, OutFailureReason))
	{
		UE_LOG(LogTTDBattle, Warning, TEXT("StartBattle failed: %s"), *OutFailureReason.ToString());
		return false;
	}

	CleanupBattle();
	GatherPlacedActors();

	++BattleInstanceId;
	ActiveLevelId = LevelId;
	BattleState = ETTDBattleState::Running;
	BattleElapsedSeconds = 0.0f;
	Currency = FMath::Max(0, LevelDefinition->StartingCurrency);
	TotalEnemyWeight = 0.0f;
	RemainingUnspawnedWeight = 0.0f;
	AliveEnemyWeight = 0.0f;
	FrozenProgress = 0.0f;
	bUseFrozenProgress = false;
	bBroadcastedInitialState = false;

	for (const FName DiagramId : LevelDefinition->StartingDiagramIds)
	{
		if (!DiagramId.IsNone())
		{
			DiagramIds.Add(DiagramId);
		}
	}

	for (const FTTDNameStack& PartStack : LevelDefinition->StartingParts)
	{
		AddPartCount(PartStack.Id, PartStack.Count);
	}

	for (const FTTDNameStack& ToyBoxStack : LevelDefinition->StartingToyBoxes)
	{
		AddToyBoxCount(ToyBoxStack.Id, ToyBoxStack.Count);
	}

	if (!SpawnCastleForLevel(*LevelDefinition))
	{
		CleanupBattle();
		OutFailureReason = BattleText(TEXT("Castle actor could not be spawned."));
		return false;
	}

	BuildSpawnQueueForLevel(*LevelDefinition);
	BroadcastStateChanged();
	BroadcastCurrencyChanged();
	BroadcastInventoryChanged();
	BroadcastProgressChanged();
	bBroadcastedInitialState = true;
	OutFailureReason = FText::GetEmpty();
	return true;
}

void UTTDBattleWorldSubsystem::EndBattle(const ETTDBattleState FinalState)
{
	if (BattleState == ETTDBattleState::Inactive)
	{
		return;
	}

	BattleState = FinalState;

	ReleaseActiveProjectiles();
	ReleaseActiveEnemies();
	ReleaseActiveBuildings();
	PendingSpawns.Reset();
	RemainingUnspawnedWeight = 0.0f;
	AliveEnemyWeight = 0.0f;

	BroadcastStateChanged();
	BroadcastProgressChanged();
	BroadcastBattleEnded(FinalState);
}

bool UTTDBattleWorldSubsystem::TryBuildOnSlot(const FName BuildingId, ATTDBuildSlotActor* Slot, FText& OutFailureReason)
{
	OutFailureReason = FText::GetEmpty();
	if (BattleState != ETTDBattleState::Running)
	{
		OutFailureReason = BattleText(TEXT("Battle is not running."));
		return false;
	}

	if (!Slot)
	{
		OutFailureReason = BattleText(TEXT("Build slot is invalid."));
		return false;
	}

	if (Slot->IsOccupied())
	{
		OutFailureReason = BattleText(TEXT("Build slot is already occupied."));
		return false;
	}

	const FTTDBuildingDefinition* BuildingDefinition = BuildingDefinitions.Find(BuildingId);
	if (!BuildingDefinition)
	{
		OutFailureReason = BattleText(TEXT("Building is not configured."));
		return false;
	}

	if (!BuildingDefinition->RequiredDiagramId.IsNone() && !DiagramIds.Contains(BuildingDefinition->RequiredDiagramId))
	{
		OutFailureReason = BattleText(TEXT("Required diagram is not available."));
		return false;
	}

	if (!HasPartCosts(BuildingDefinition->PartCosts))
	{
		OutFailureReason = BattleText(TEXT("Not enough parts."));
		return false;
	}

	UClass* BuildingClass = BuildingDefinition->BuildingClass.LoadSynchronous();
	if (!BuildingClass)
	{
		BuildingClass = ATTDBattleBuildingActor::StaticClass();
	}

	bool bFromPool = false;
	AActor* Actor = nullptr;
	if (UTTDObjectPoolWorldSubsystem* PoolSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UTTDObjectPoolWorldSubsystem>() : nullptr)
	{
		Actor = PoolSubsystem->AcquireActor(BuildingClass, Slot->GetActorTransform(), this, bFromPool);
	}

	ATTDBattleBuildingActor* Building = Cast<ATTDBattleBuildingActor>(Actor);
	if (!Building)
	{
		OutFailureReason = BattleText(TEXT("Building actor could not be spawned."));
		return false;
	}

	ApplyPartCosts(BuildingDefinition->PartCosts);
	const FTTDBattleBuildingRuntimeStats RuntimeStats = BuildRuntimeStatsForSlot(*BuildingDefinition, Slot);
	Building->InitializeBuilding(BuildingId, *BuildingDefinition, RuntimeStats, Slot);
	Slot->SetOccupyingBuilding(Building);
	ActiveBuildings.Add(Building);
	RegisterBattleTarget(Building);

	BroadcastInventoryChanged();
	BroadcastBuildingPlaced(BuildingId, Slot->GetSlotId());
	return true;
}

bool UTTDBattleWorldSubsystem::BuyToyBox(const FName ToyBoxId, FText& OutFailureReason)
{
	OutFailureReason = FText::GetEmpty();
	const FTTDToyBoxRewardDefinition* ToyBoxDefinition = ToyBoxRewardDefinitions.Find(ToyBoxId);
	if (!ToyBoxDefinition)
	{
		OutFailureReason = BattleText(TEXT("Toy box is not configured."));
		return false;
	}

	if (Currency < ToyBoxDefinition->PurchaseCost)
	{
		OutFailureReason = BattleText(TEXT("Not enough currency."));
		return false;
	}

	Currency -= ToyBoxDefinition->PurchaseCost;
	AddToyBoxCount(ToyBoxId, 1);
	BroadcastCurrencyChanged();
	BroadcastInventoryChanged();
	return true;
}

bool UTTDBattleWorldSubsystem::OpenToyBox(FName ToyBoxId, TArray<FTTDNameStack>& OutRewards, FText& OutFailureReason)
{
	OutRewards.Reset();
	OutFailureReason = FText::GetEmpty();

	const FTTDToyBoxRewardDefinition* ToyBoxDefinition = ToyBoxRewardDefinitions.Find(ToyBoxId);
	if (!ToyBoxDefinition)
	{
		OutFailureReason = BattleText(TEXT("Toy box is not configured."));
		return false;
	}

	int32* ToyBoxCount = ToyBoxCounts.Find(ToyBoxId);
	if (!ToyBoxCount || *ToyBoxCount <= 0)
	{
		OutFailureReason = BattleText(TEXT("Toy box is not available."));
		return false;
	}

	float TotalWeight = 0.0f;
	for (const FTTDToyBoxRewardEntry& Entry : ToyBoxDefinition->RewardEntries)
	{
		TotalWeight += FMath::Max(0.0f, Entry.Weight);
	}

	if (TotalWeight <= 0.0f)
	{
		OutFailureReason = BattleText(TEXT("Toy box has no valid rewards."));
		return false;
	}

	--(*ToyBoxCount);
	if (*ToyBoxCount <= 0)
	{
		ToyBoxCounts.Remove(ToyBoxId);
	}

	TMap<FName, int32> RewardCounts;
	for (int32 RollIndex = 0; RollIndex < ToyBoxDefinition->RollCount; ++RollIndex)
	{
		const float Roll = RewardRandomStream.FRandRange(0.0f, TotalWeight);
		float Cursor = 0.0f;
		const FTTDToyBoxRewardEntry* SelectedEntry = nullptr;
		for (const FTTDToyBoxRewardEntry& Entry : ToyBoxDefinition->RewardEntries)
		{
			Cursor += FMath::Max(0.0f, Entry.Weight);
			if (Roll <= Cursor)
			{
				SelectedEntry = &Entry;
				break;
			}
		}

		if (!SelectedEntry)
		{
			SelectedEntry = ToyBoxDefinition->RewardEntries.Last().Weight > 0.0f ? &ToyBoxDefinition->RewardEntries.Last() : nullptr;
		}

		if (SelectedEntry && !SelectedEntry->PartId.IsNone())
		{
			const int32 MinQuantity = FMath::Max(1, SelectedEntry->MinQuantity);
			const int32 MaxQuantity = FMath::Max(MinQuantity, SelectedEntry->MaxQuantity);
			const int32 Quantity = RewardRandomStream.RandRange(MinQuantity, MaxQuantity);
			RewardCounts.FindOrAdd(SelectedEntry->PartId) += Quantity;
			AddPartCount(SelectedEntry->PartId, Quantity);
		}
	}

	OutRewards = StackArrayFromMap(RewardCounts);
	BroadcastInventoryChanged();
	return true;
}

float UTTDBattleWorldSubsystem::GetProgress() const
{
	if (bUseFrozenProgress)
	{
		return FrozenProgress;
	}

	if (TotalEnemyWeight <= 0.0f)
	{
		return BattleState == ETTDBattleState::Victory ? 1.0f : 0.0f;
	}

	return FMath::Clamp(1.0f - ((RemainingUnspawnedWeight + AliveEnemyWeight) / TotalEnemyWeight), 0.0f, 1.0f);
}

float UTTDBattleWorldSubsystem::GetCastleCurrentHealth() const
{
	return CastleActor ? CastleActor->GetCurrentHealth() : 0.0f;
}

float UTTDBattleWorldSubsystem::GetCastleMaxHealth() const
{
	return CastleActor ? CastleActor->GetMaxHealth() : 0.0f;
}

TArray<FTTDBuildingDefinition> UTTDBattleWorldSubsystem::GetBuildingDefinitions() const
{
	TArray<FTTDBuildingDefinition> Result;
	BuildingDefinitions.GenerateValueArray(Result);
	return Result;
}

TArray<FTTDToyBoxRewardDefinition> UTTDBattleWorldSubsystem::GetToyBoxDefinitions() const
{
	TArray<FTTDToyBoxRewardDefinition> Result;
	ToyBoxRewardDefinitions.GenerateValueArray(Result);
	return Result;
}

TArray<FName> UTTDBattleWorldSubsystem::GetDiagramIds() const
{
	TArray<FName> Result = DiagramIds.Array();
	return Result;
}

TArray<FTTDNameStack> UTTDBattleWorldSubsystem::GetPartStacks() const
{
	return StackArrayFromMap(PartCounts);
}

TArray<FTTDNameStack> UTTDBattleWorldSubsystem::GetToyBoxStacks() const
{
	return StackArrayFromMap(ToyBoxCounts);
}

void UTTDBattleWorldSubsystem::SetRewardRandomSeed(const int32 Seed)
{
	RewardRandomStream.Initialize(Seed);
}

void UTTDBattleWorldSubsystem::HandleCastleDestroyed(ATTDBattleCastleActor* DestroyedCastle)
{
	if (DestroyedCastle && DestroyedCastle == CastleActor)
	{
		EndBattle(ETTDBattleState::Defeat);
	}
}

void UTTDBattleWorldSubsystem::HandleBuildingDestroyed(ATTDBattleBuildingActor* DestroyedBuilding)
{
	if (!DestroyedBuilding)
	{
		return;
	}

	if (ATTDBuildSlotActor* Slot = DestroyedBuilding->GetOwningSlot())
	{
		Slot->ClearOccupyingBuilding(DestroyedBuilding);
	}

	ActiveBuildings.Remove(DestroyedBuilding);
	UnregisterBattleTarget(DestroyedBuilding);

	if (UTTDObjectPoolWorldSubsystem* PoolSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UTTDObjectPoolWorldSubsystem>() : nullptr)
	{
		PoolSubsystem->ReleaseObject(DestroyedBuilding);
	}
	else
	{
		DestroyedBuilding->Destroy();
	}
}

void UTTDBattleWorldSubsystem::HandleEnemyKilled(ATTDBattleEnemyActor* Enemy)
{
	if (!Enemy)
	{
		return;
	}

	const int32 RemovedCount = ActiveEnemies.Remove(Enemy);
	if (RemovedCount <= 0)
	{
		return;
	}

	AliveEnemyWeight = FMath::Max(0.0f, AliveEnemyWeight - Enemy->GetProgressWeight());
	Currency += Enemy->GetCurrencyDrop();

	if (UTTDObjectPoolWorldSubsystem* PoolSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UTTDObjectPoolWorldSubsystem>() : nullptr)
	{
		PoolSubsystem->ReleaseObject(Enemy);
	}
	else
	{
		Enemy->Destroy();
	}

	BroadcastCurrencyChanged();
	BroadcastProgressChanged();
	CheckVictoryCondition();
}

void UTTDBattleWorldSubsystem::RegisterProjectile(ATTDBattleProjectileActor* Projectile, const int32 InBattleInstanceId)
{
	if (Projectile && IsBattleInstanceCurrent(InBattleInstanceId))
	{
		ActiveProjectiles.AddUnique(Projectile);
	}
}

void UTTDBattleWorldSubsystem::UnregisterProjectile(ATTDBattleProjectileActor* Projectile)
{
	ActiveProjectiles.Remove(Projectile);
}

bool UTTDBattleWorldSubsystem::IsBattleInstanceCurrent(const int32 InBattleInstanceId) const
{
	return BattleState == ETTDBattleState::Running && InBattleInstanceId == BattleInstanceId;
}

UObject* UTTDBattleWorldSubsystem::FindNearestBattleTarget(const FVector& FromLocation) const
{
	UObject* BestTarget = nullptr;
	float BestDistanceSquared = TNumericLimits<float>::Max();

	for (const TObjectPtr<UObject>& TargetPtr : BattleTargets)
	{
		UObject* Target = TargetPtr.Get();
		if (!IsValid(Target) || !Target->GetClass()->ImplementsInterface(UTTDBattleTargetInterface::StaticClass()))
		{
			continue;
		}

		if (!IsBattleTargetAlive(Target))
		{
			continue;
		}

		const FVector TargetLocation = GetBattleTargetLocation(Target);
		const float DistanceSquared = FVector::DistSquared2D(FromLocation, TargetLocation);
		if (DistanceSquared < BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			BestTarget = Target;
		}
	}

	return BestTarget;
}

ATTDBattleEnemyActor* UTTDBattleWorldSubsystem::FindNearestEnemy(const FVector& FromLocation, const float MaxRange) const
{
	ATTDBattleEnemyActor* BestEnemy = nullptr;
	float BestDistanceSquared = FMath::Square(FMath::Max(0.0f, MaxRange));

	for (ATTDBattleEnemyActor* Enemy : ActiveEnemies)
	{
		if (!IsValid(Enemy) || !Enemy->IsAlive())
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared2D(FromLocation, Enemy->GetActorLocation());
		if (DistanceSquared <= BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			BestEnemy = Enemy;
		}
	}

	return BestEnemy;
}

const FTTDBuildingDefinition* UTTDBattleWorldSubsystem::FindBuildingDefinition(const FName BuildingId) const
{
	return BuildingDefinitions.Find(BuildingId);
}

const FTTDToyBoxRewardDefinition* UTTDBattleWorldSubsystem::FindToyBoxDefinition(const FName ToyBoxId) const
{
	return ToyBoxRewardDefinitions.Find(ToyBoxId);
}

FTTDBattleBuildingRuntimeStats UTTDBattleWorldSubsystem::BuildRuntimeStatsForSlot(const FTTDBuildingDefinition& BuildingDefinition, const ATTDBuildSlotActor* Slot) const
{
	FTTDBattleBuildingRuntimeStats RuntimeStats = BuildingDefinition.BaseStats;
	if (!Slot || Slot->GetHeightEffectId().IsNone())
	{
		return RuntimeStats;
	}

	const FTTDBattleHeightEffectDefinition* HeightEffect = HeightEffectDefinitions.Find(Slot->GetHeightEffectId());
	if (!HeightEffect)
	{
		return RuntimeStats;
	}

	for (const FTTDBattleAttributeModifier& Modifier : HeightEffect->Modifiers)
	{
		switch (Modifier.Attribute)
		{
		case ETTDBattleAttribute::AttackDamage:
			ApplyModifier(RuntimeStats.AttackDamage, Modifier);
			break;
		case ETTDBattleAttribute::AttackRange:
			ApplyModifier(RuntimeStats.AttackRange, Modifier);
			break;
		case ETTDBattleAttribute::AttackInterval:
			ApplyModifier(RuntimeStats.AttackInterval, Modifier);
			break;
		case ETTDBattleAttribute::MaxHealth:
			ApplyModifier(RuntimeStats.MaxHealth, Modifier);
			break;
		case ETTDBattleAttribute::ProjectileSpeed:
			ApplyModifier(RuntimeStats.ProjectileSpeed, Modifier);
			break;
		default:
			break;
		}
	}

	RuntimeStats.MaxHealth = FMath::Max(1.0f, RuntimeStats.MaxHealth);
	RuntimeStats.AttackDamage = FMath::Max(0.0f, RuntimeStats.AttackDamage);
	RuntimeStats.AttackRange = FMath::Max(0.0f, RuntimeStats.AttackRange);
	RuntimeStats.AttackInterval = FMath::Max(0.01f, RuntimeStats.AttackInterval);
	RuntimeStats.ProjectileSpeed = FMath::Max(1.0f, RuntimeStats.ProjectileSpeed);
	return RuntimeStats;
}

void UTTDBattleWorldSubsystem::LoadDefinitionsFromSettings()
{
	LevelDefinitions.Reset();
	WaveDefinitions.Reset();
	EnemyDefinitions.Reset();
	BuildingDefinitions.Reset();
	ToyBoxRewardDefinitions.Reset();
	HeightEffectDefinitions.Reset();

	const UTTDDeveloperSettings* Settings = GetDefault<UTTDDeveloperSettings>();
	if (!Settings)
	{
		return;
	}

	LoadRowsIntoMap<FTTDBattleLevelDefinition>(
		Settings->GetDataTableForTag(TAG_TTD_DataTable_Battle_Levels),
		LevelDefinitions,
		[](FTTDBattleLevelDefinition& Row, const FName RowName)
		{
			if (Row.LevelId.IsNone())
			{
				Row.LevelId = RowName;
			}
		});

	LoadRowsIntoMap<FTTDWaveDefinition>(
		Settings->GetDataTableForTag(TAG_TTD_DataTable_Battle_Waves),
		WaveDefinitions,
		[](FTTDWaveDefinition& Row, const FName RowName)
		{
			if (Row.WaveId.IsNone())
			{
				Row.WaveId = RowName;
			}
		});

	LoadRowsIntoMap<FTTDEnemyDefinition>(
		Settings->GetDataTableForTag(TAG_TTD_DataTable_Battle_Enemies),
		EnemyDefinitions,
		[](FTTDEnemyDefinition& Row, const FName RowName)
		{
			if (Row.EnemyId.IsNone())
			{
				Row.EnemyId = RowName;
			}
		});

	LoadRowsIntoMap<FTTDBuildingDefinition>(
		Settings->GetDataTableForTag(TAG_TTD_DataTable_Battle_Buildings),
		BuildingDefinitions,
		[](FTTDBuildingDefinition& Row, const FName RowName)
		{
			if (Row.BuildingId.IsNone())
			{
				Row.BuildingId = RowName;
			}
		});

	LoadRowsIntoMap<FTTDToyBoxRewardDefinition>(
		Settings->GetDataTableForTag(TAG_TTD_DataTable_Battle_ToyBoxRewards),
		ToyBoxRewardDefinitions,
		[](FTTDToyBoxRewardDefinition& Row, const FName RowName)
		{
			if (Row.ToyBoxId.IsNone())
			{
				Row.ToyBoxId = RowName;
			}
		});

	LoadRowsIntoMap<FTTDBattleHeightEffectDefinition>(
		Settings->GetDataTableForTag(TAG_TTD_DataTable_Battle_HeightEffects),
		HeightEffectDefinitions,
		[](FTTDBattleHeightEffectDefinition& Row, const FName RowName)
		{
			if (Row.HeightEffectId.IsNone())
			{
				Row.HeightEffectId = RowName;
			}
		});
}

void UTTDBattleWorldSubsystem::GatherPlacedActors()
{
	SpawnPoints.Reset();
	BuildSlots.Reset();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TActorIterator<ATTDBattleSpawnPointActor> It(World); It; ++It)
	{
		SpawnPoints.Add(*It);
	}

	for (TActorIterator<ATTDBuildSlotActor> It(World); It; ++It)
	{
		BuildSlots.Add(*It);
	}
}

bool UTTDBattleWorldSubsystem::ValidateBattleStart(const FName LevelId, FText& OutFailureReason) const
{
	if (LevelId.IsNone())
	{
		OutFailureReason = BattleText(TEXT("Default battle level id is empty."));
		return false;
	}

	const FTTDBattleLevelDefinition* LevelDefinition = LevelDefinitions.Find(LevelId);
	if (!LevelDefinition)
	{
		OutFailureReason = FText::Format(BattleText(TEXT("Battle level {0} is not configured.")), FText::FromName(LevelId));
		return false;
	}

	return ValidateLevelDefinition(LevelId, *LevelDefinition, OutFailureReason)
		&& ValidateProjectileBuildingDefinitions(OutFailureReason);
}

bool UTTDBattleWorldSubsystem::ValidateLevelDefinition(const FName LevelId, const FTTDBattleLevelDefinition& LevelDefinition, FText& OutFailureReason) const
{
	if (SpawnPoints.IsEmpty())
	{
		OutFailureReason = BattleText(TEXT("Battle level requires at least one spawn point in the world."));
		return false;
	}

	if (BuildSlots.IsEmpty())
	{
		OutFailureReason = BattleText(TEXT("Battle level requires at least one build slot in the world."));
		return false;
	}

	UClass* CastleClass = LevelDefinition.CastleClass.LoadSynchronous();
	if (CastleClass && !CastleClass->IsChildOf(ATTDBattleCastleActor::StaticClass()))
	{
		OutFailureReason = FText::Format(BattleText(TEXT("Castle class for level {0} must derive from ATTDBattleCastleActor.")), FText::FromName(LevelId));
		return false;
	}

	for (const FName WaveId : LevelDefinition.WaveIds)
	{
		const FTTDWaveDefinition* WaveDefinition = WaveDefinitions.Find(WaveId);
		if (!WaveDefinition)
		{
			OutFailureReason = FText::Format(BattleText(TEXT("Battle level {0} references missing wave {1}.")), FText::FromName(LevelId), FText::FromName(WaveId));
			return false;
		}

		for (const FTTDWaveEnemyEntry& EnemyEntry : WaveDefinition->EnemyEntries)
		{
			if (!ValidateEnemyDefinition(EnemyEntry.EnemyId, OutFailureReason))
			{
				return false;
			}

			if (EnemyEntry.Count > 0 && !HasSpawnPointForGroup(EnemyEntry.SpawnGroup))
			{
				OutFailureReason = EnemyEntry.SpawnGroup.IsNone()
					? BattleText(TEXT("Wave requires a spawn point, but none are available."))
					: FText::Format(BattleText(TEXT("Wave requires spawn group {0}, but no matching spawn point exists.")), FText::FromName(EnemyEntry.SpawnGroup));
				return false;
			}
		}
	}

	OutFailureReason = FText::GetEmpty();
	return true;
}

bool UTTDBattleWorldSubsystem::ValidateEnemyDefinition(const FName EnemyId, FText& OutFailureReason) const
{
	const FTTDEnemyDefinition* EnemyDefinition = EnemyDefinitions.Find(EnemyId);
	if (!EnemyDefinition)
	{
		OutFailureReason = FText::Format(BattleText(TEXT("Enemy {0} is not configured.")), FText::FromName(EnemyId));
		return false;
	}

	UClass* EnemyClass = ResolveEnemyClass(*EnemyDefinition);
	if (!IsPoolableActorClass(EnemyClass, ATTDBattleEnemyActor::StaticClass()))
	{
		OutFailureReason = FText::Format(BattleText(TEXT("Enemy class for {0} must derive from ATTDBattleEnemyActor and implement pooled object interface.")), FText::FromName(EnemyId));
		return false;
	}

	return true;
}

bool UTTDBattleWorldSubsystem::ValidateProjectileBuildingDefinitions(FText& OutFailureReason) const
{
	for (const TPair<FName, FTTDBuildingDefinition>& Pair : BuildingDefinitions)
	{
		const FTTDBuildingDefinition& BuildingDefinition = Pair.Value;
		if (BuildingDefinition.AttackMode != ETTDBuildingAttackMode::Projectile)
		{
			continue;
		}

		UClass* ProjectileClass = BuildingDefinition.ProjectileClass.LoadSynchronous();
		if (!ProjectileClass)
		{
			ProjectileClass = ATTDBattleProjectileActor::StaticClass();
		}

		if (!IsPoolableActorClass(ProjectileClass, ATTDBattleProjectileActor::StaticClass()))
		{
			OutFailureReason = FText::Format(BattleText(TEXT("Projectile class for building {0} must derive from ATTDBattleProjectileActor and implement pooled object interface.")), FText::FromName(BuildingDefinition.BuildingId));
			return false;
		}
	}

	return true;
}

bool UTTDBattleWorldSubsystem::HasSpawnPointForGroup(const FName SpawnGroup) const
{
	for (const ATTDBattleSpawnPointActor* SpawnPoint : SpawnPoints)
	{
		if (IsValid(SpawnPoint) && (SpawnGroup.IsNone() || SpawnPoint->GetSpawnGroup() == SpawnGroup))
		{
			return true;
		}
	}

	return false;
}

bool UTTDBattleWorldSubsystem::IsPoolableActorClass(const UClass* ActorClass, const UClass* RequiredBaseClass) const
{
	return ActorClass
		&& RequiredBaseClass
		&& ActorClass->IsChildOf(RequiredBaseClass)
		&& !ActorClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists)
		&& ActorClass->ImplementsInterface(UTTDPooledObjectInterface::StaticClass());
}

UClass* UTTDBattleWorldSubsystem::ResolveEnemyClass(const FTTDEnemyDefinition& EnemyDefinition) const
{
	UClass* EnemyClass = EnemyDefinition.EnemyClass.LoadSynchronous();
	return EnemyClass ? EnemyClass : ATTDBattleEnemyActor::StaticClass();
}

void UTTDBattleWorldSubsystem::EnterConfigurationError(const FText& FailureReason)
{
	FrozenProgress = GetProgress();
	bUseFrozenProgress = true;
	UE_LOG(LogTTDBattle, Error, TEXT("Battle entered configuration error: %s"), *FailureReason.ToString());
	EndBattle(ETTDBattleState::ConfigurationError);
}

void UTTDBattleWorldSubsystem::ReleaseActiveEnemies()
{
	for (int32 Index = ActiveEnemies.Num() - 1; Index >= 0; --Index)
	{
		if (ATTDBattleEnemyActor* Enemy = ActiveEnemies[Index])
		{
			ReleaseBattleActor(Enemy);
		}
	}

	ActiveEnemies.Reset();
}

void UTTDBattleWorldSubsystem::ReleaseActiveBuildings()
{
	for (int32 Index = ActiveBuildings.Num() - 1; Index >= 0; --Index)
	{
		ATTDBattleBuildingActor* Building = ActiveBuildings[Index];
		if (!Building)
		{
			continue;
		}

		if (ATTDBuildSlotActor* Slot = Building->GetOwningSlot())
		{
			Slot->ClearOccupyingBuilding(Building);
		}

		UnregisterBattleTarget(Building);
		ReleaseBattleActor(Building);
	}

	ActiveBuildings.Reset();
}

void UTTDBattleWorldSubsystem::ReleaseActiveProjectiles()
{
	for (int32 Index = ActiveProjectiles.Num() - 1; Index >= 0; --Index)
	{
		ATTDBattleProjectileActor* Projectile = ActiveProjectiles[Index];
		if (!Projectile)
		{
			continue;
		}

		ActiveProjectiles.RemoveAtSwap(Index);
		ReleaseBattleActor(Projectile);
	}

	ActiveProjectiles.Reset();
}

void UTTDBattleWorldSubsystem::ReleaseBattleActor(AActor* Actor) const
{
	if (!IsValid(Actor))
	{
		return;
	}

	if (UTTDObjectPoolWorldSubsystem* PoolSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UTTDObjectPoolWorldSubsystem>() : nullptr)
	{
		PoolSubsystem->ReleaseObject(Actor);
	}
	else
	{
		Actor->Destroy();
	}
}

void UTTDBattleWorldSubsystem::CleanupBattle()
{
	ReleaseActiveProjectiles();
	ReleaseActiveEnemies();
	ReleaseActiveBuildings();

	if (IsValid(Cast<AActor>(CastleActor)))
	{
		Cast<AActor>(CastleActor)->Destroy();
	}

	CastleActor = nullptr;
	SpawnPoints.Reset();
	BuildSlots.Reset();
	BattleTargets.Reset();
	PendingSpawns.Reset();
	DiagramIds.Reset();
	PartCounts.Reset();
	ToyBoxCounts.Reset();
	ActiveLevelId = NAME_None;
	BattleState = ETTDBattleState::Inactive;
	BattleElapsedSeconds = 0.0f;
	TotalEnemyWeight = 0.0f;
	RemainingUnspawnedWeight = 0.0f;
	AliveEnemyWeight = 0.0f;
	FrozenProgress = 0.0f;
	Currency = 0;
	bUseFrozenProgress = false;
}

bool UTTDBattleWorldSubsystem::SpawnCastleForLevel(const FTTDBattleLevelDefinition& LevelDefinition)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	UClass* CastleClass = LevelDefinition.CastleClass.LoadSynchronous();
	if (!CastleClass)
	{
		CastleClass = ATTDBattleCastleActor::StaticClass();
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	CastleActor = World->SpawnActor<ATTDBattleCastleActor>(CastleClass, LevelDefinition.CastleTransform, SpawnParameters);
	if (!CastleActor)
	{
		UE_LOG(LogTTDBattle, Warning, TEXT("StartBattle failed: castle class %s did not spawn a battle castle."), *GetNameSafe(CastleClass));
		return false;
	}

	CastleActor->InitializeCastle(LevelDefinition.CastleMaxHealth);
	RegisterBattleTarget(CastleActor);
	return true;
}

void UTTDBattleWorldSubsystem::BuildSpawnQueueForLevel(const FTTDBattleLevelDefinition& LevelDefinition)
{
	PendingSpawns.Reset();
	float WaveStartTime = 0.0f;

	for (const FName WaveId : LevelDefinition.WaveIds)
	{
		const FTTDWaveDefinition* WaveDefinition = WaveDefinitions.Find(WaveId);
		if (!WaveDefinition)
		{
			UE_LOG(LogTTDBattle, Warning, TEXT("Battle level %s references missing wave %s."), *LevelDefinition.LevelId.ToString(), *WaveId.ToString());
			continue;
		}

		WaveStartTime += FMath::Max(0.0f, WaveDefinition->DelayBeforeWave);
		float WaveDuration = 0.0f;
		for (const FTTDWaveEnemyEntry& EnemyEntry : WaveDefinition->EnemyEntries)
		{
			const FTTDEnemyDefinition* EnemyDefinition = EnemyDefinitions.Find(EnemyEntry.EnemyId);
			if (!EnemyDefinition)
			{
				UE_LOG(LogTTDBattle, Warning, TEXT("Wave %s references missing enemy %s."), *WaveId.ToString(), *EnemyEntry.EnemyId.ToString());
				continue;
			}

			const float EntryInterval = FMath::Max(0.01f, EnemyEntry.SpawnInterval);
			for (int32 SpawnIndex = 0; SpawnIndex < EnemyEntry.Count; ++SpawnIndex)
			{
				FTTDBattlePendingSpawn PendingSpawn;
				PendingSpawn.EnemyId = EnemyEntry.EnemyId;
				PendingSpawn.SpawnGroup = EnemyEntry.SpawnGroup;
				PendingSpawn.SpawnTimeSeconds = WaveStartTime + SpawnIndex * EntryInterval;
				PendingSpawn.ProgressWeight = FMath::Max(0.0f, EnemyDefinition->ProgressWeight);
				PendingSpawns.Add(PendingSpawn);
				TotalEnemyWeight += PendingSpawn.ProgressWeight;
				RemainingUnspawnedWeight += PendingSpawn.ProgressWeight;
				WaveDuration = FMath::Max(WaveDuration, SpawnIndex * EntryInterval);
			}
		}

		WaveStartTime += WaveDuration;
	}

	PendingSpawns.Sort(
		[](const FTTDBattlePendingSpawn& Left, const FTTDBattlePendingSpawn& Right)
		{
			return Left.SpawnTimeSeconds < Right.SpawnTimeSeconds;
		});
}

void UTTDBattleWorldSubsystem::SpawnDueEnemies()
{
	bool bSpawnedAnyEnemy = false;
	while (!PendingSpawns.IsEmpty() && PendingSpawns[0].SpawnTimeSeconds <= BattleElapsedSeconds)
	{
		const FTTDBattlePendingSpawn PendingSpawn = PendingSpawns[0];
		FText FailureReason;
		if (!SpawnEnemy(PendingSpawn, FailureReason))
		{
			EnterConfigurationError(FailureReason);
			return;
		}

		PendingSpawns.RemoveAt(0);
		RemainingUnspawnedWeight = FMath::Max(0.0f, RemainingUnspawnedWeight - PendingSpawn.ProgressWeight);
		bSpawnedAnyEnemy = true;
	}

	if (bSpawnedAnyEnemy)
	{
		BroadcastProgressChanged();
	}
}

bool UTTDBattleWorldSubsystem::SpawnEnemy(const FTTDBattlePendingSpawn& PendingSpawn, FText& OutFailureReason)
{
	const FTTDEnemyDefinition* EnemyDefinition = EnemyDefinitions.Find(PendingSpawn.EnemyId);
	if (!EnemyDefinition)
	{
		OutFailureReason = FText::Format(BattleText(TEXT("Enemy {0} is not configured.")), FText::FromName(PendingSpawn.EnemyId));
		return false;
	}

	UClass* EnemyClass = ResolveEnemyClass(*EnemyDefinition);
	if (!IsPoolableActorClass(EnemyClass, ATTDBattleEnemyActor::StaticClass()))
	{
		OutFailureReason = FText::Format(BattleText(TEXT("Enemy class for {0} is invalid.")), FText::FromName(PendingSpawn.EnemyId));
		return false;
	}

	const ATTDBattleSpawnPointActor* SpawnPoint = PickSpawnPoint(PendingSpawn.SpawnGroup);
	if (!SpawnPoint)
	{
		OutFailureReason = PendingSpawn.SpawnGroup.IsNone()
			? BattleText(TEXT("No spawn point is available."))
			: FText::Format(BattleText(TEXT("No spawn point found for group {0}.")), FText::FromName(PendingSpawn.SpawnGroup));
		return false;
	}

	const FTransform SpawnTransform = SpawnPoint->GetActorTransform();
	AActor* Actor = nullptr;
	bool bFromPool = false;
	if (UTTDObjectPoolWorldSubsystem* PoolSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UTTDObjectPoolWorldSubsystem>() : nullptr)
	{
		Actor = PoolSubsystem->AcquireActor(EnemyClass, SpawnTransform, this, bFromPool);
	}

	ATTDBattleEnemyActor* Enemy = Cast<ATTDBattleEnemyActor>(Actor);
	if (!Enemy)
	{
		OutFailureReason = FText::Format(BattleText(TEXT("Enemy class {0} could not be acquired from the object pool.")), FText::FromString(GetNameSafe(EnemyClass)));
		UE_LOG(LogTTDBattle, Warning, TEXT("%s"), *OutFailureReason.ToString());
		return false;
	}

	Enemy->InitializeEnemy(*EnemyDefinition);
	ActiveEnemies.Add(Enemy);
	AliveEnemyWeight += Enemy->GetProgressWeight();
	return true;
}

ATTDBattleSpawnPointActor* UTTDBattleWorldSubsystem::PickSpawnPoint(const FName SpawnGroup) const
{
	TArray<ATTDBattleSpawnPointActor*> MatchingPoints;
	for (ATTDBattleSpawnPointActor* SpawnPoint : SpawnPoints)
	{
		if (!IsValid(SpawnPoint))
		{
			continue;
		}

		if (SpawnGroup.IsNone() || SpawnPoint->GetSpawnGroup() == SpawnGroup)
		{
			MatchingPoints.Add(SpawnPoint);
		}
	}

	if (MatchingPoints.IsEmpty())
	{
		return nullptr;
	}

	return MatchingPoints[RewardRandomStream.RandRange(0, MatchingPoints.Num() - 1)];
}

void UTTDBattleWorldSubsystem::CheckVictoryCondition()
{
	if (BattleState == ETTDBattleState::Running && PendingSpawns.IsEmpty() && ActiveEnemies.IsEmpty())
	{
		EndBattle(ETTDBattleState::Victory);
	}
}

void UTTDBattleWorldSubsystem::RegisterBattleTarget(UObject* Target)
{
	if (Target)
	{
		BattleTargets.AddUnique(Target);
	}
}

void UTTDBattleWorldSubsystem::UnregisterBattleTarget(UObject* Target)
{
	BattleTargets.Remove(Target);
}

bool UTTDBattleWorldSubsystem::HasPartCosts(const TArray<FTTDNameStack>& PartCosts) const
{
	for (const FTTDNameStack& PartCost : PartCosts)
	{
		const int32 RequiredCount = FMath::Max(0, PartCost.Count);
		if (RequiredCount <= 0)
		{
			continue;
		}

		const int32 AvailableCount = PartCounts.FindRef(PartCost.Id);
		if (AvailableCount < RequiredCount)
		{
			return false;
		}
	}

	return true;
}

void UTTDBattleWorldSubsystem::ApplyPartCosts(const TArray<FTTDNameStack>& PartCosts)
{
	for (const FTTDNameStack& PartCost : PartCosts)
	{
		if (PartCost.Id.IsNone() || PartCost.Count <= 0)
		{
			continue;
		}

		int32& Count = PartCounts.FindOrAdd(PartCost.Id);
		Count = FMath::Max(0, Count - PartCost.Count);
		if (Count <= 0)
		{
			PartCounts.Remove(PartCost.Id);
		}
	}
}

void UTTDBattleWorldSubsystem::AddPartCount(const FName PartId, const int32 Count)
{
	if (!PartId.IsNone() && Count > 0)
	{
		PartCounts.FindOrAdd(PartId) += Count;
	}
}

void UTTDBattleWorldSubsystem::AddToyBoxCount(const FName ToyBoxId, const int32 Count)
{
	if (!ToyBoxId.IsNone() && Count > 0)
	{
		ToyBoxCounts.FindOrAdd(ToyBoxId) += Count;
	}
}

void UTTDBattleWorldSubsystem::BroadcastStateChanged() const
{
	if (const UWorld* World = GetWorld())
	{
		if (const UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (UGameplayMessageSubsystem* MessageSubsystem = GameInstance->GetSubsystem<UGameplayMessageSubsystem>())
			{
				FTTDBattleStateChangedMessage Message;
				Message.LevelId = ActiveLevelId;
				Message.State = BattleState;
				MessageSubsystem->BroadcastMessage(TAG_TTD_Message_Battle_State_Changed, Message);
			}
		}
	}
}

void UTTDBattleWorldSubsystem::BroadcastProgressChanged() const
{
	if (const UWorld* World = GetWorld())
	{
		if (const UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (UGameplayMessageSubsystem* MessageSubsystem = GameInstance->GetSubsystem<UGameplayMessageSubsystem>())
			{
				FTTDBattleProgressChangedMessage Message;
				Message.Progress = GetProgress();
				Message.RemainingWeight = GetRemainingEnemyWeight();
				Message.TotalWeight = TotalEnemyWeight;
				MessageSubsystem->BroadcastMessage(TAG_TTD_Message_Battle_Progress_Changed, Message);
			}
		}
	}
}

void UTTDBattleWorldSubsystem::BroadcastCurrencyChanged() const
{
	if (const UWorld* World = GetWorld())
	{
		if (const UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (UGameplayMessageSubsystem* MessageSubsystem = GameInstance->GetSubsystem<UGameplayMessageSubsystem>())
			{
				FTTDBattleCurrencyChangedMessage Message;
				Message.Currency = Currency;
				MessageSubsystem->BroadcastMessage(TAG_TTD_Message_Battle_Currency_Changed, Message);
			}
		}
	}
}

void UTTDBattleWorldSubsystem::BroadcastInventoryChanged() const
{
	if (const UWorld* World = GetWorld())
	{
		if (const UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (UGameplayMessageSubsystem* MessageSubsystem = GameInstance->GetSubsystem<UGameplayMessageSubsystem>())
			{
				FTTDBattleInventoryChangedMessage Message;
				Message.DiagramIds = DiagramIds.Array();
				Message.Parts = StackArrayFromMap(PartCounts);
				Message.ToyBoxes = StackArrayFromMap(ToyBoxCounts);
				MessageSubsystem->BroadcastMessage(TAG_TTD_Message_Battle_Inventory_Changed, Message);
			}
		}
	}
}

void UTTDBattleWorldSubsystem::BroadcastBuildingPlaced(const FName BuildingId, const FName SlotId) const
{
	if (const UWorld* World = GetWorld())
	{
		if (const UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (UGameplayMessageSubsystem* MessageSubsystem = GameInstance->GetSubsystem<UGameplayMessageSubsystem>())
			{
				FTTDBattleBuildingPlacedMessage Message;
				Message.BuildingId = BuildingId;
				Message.SlotId = SlotId;
				MessageSubsystem->BroadcastMessage(TAG_TTD_Message_Battle_Building_Placed, Message);
			}
		}
	}
}

void UTTDBattleWorldSubsystem::BroadcastBattleEnded(const ETTDBattleState FinalState) const
{
	if (const UWorld* World = GetWorld())
	{
		if (const UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (UGameplayMessageSubsystem* MessageSubsystem = GameInstance->GetSubsystem<UGameplayMessageSubsystem>())
			{
				FTTDBattleEndedMessage Message;
				Message.LevelId = ActiveLevelId;
				Message.FinalState = FinalState;
				MessageSubsystem->BroadcastMessage(TAG_TTD_Message_Battle_Ended, Message);
			}
		}
	}
}
