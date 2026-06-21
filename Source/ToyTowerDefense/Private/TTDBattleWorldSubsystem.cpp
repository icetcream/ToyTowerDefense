#include "TTDBattleWorldSubsystem.h"

#include "Battle/TTDBattleBackgroundActor.h"
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
	const FName LegacyBasicToyBoxId(TEXT("BasicBox"));
	const FName BasicToyBoxId(TEXT("BasicToyBox"));

	FText BattleText(const TCHAR* Value)
	{
		return FText::FromString(Value);
	}

	FName NormalizeToyBoxId(const FName ToyBoxId)
	{
		return ToyBoxId == LegacyBasicToyBoxId ? BasicToyBoxId : ToyBoxId;
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
	if (DeltaTime <= 0.0f)
	{
		return;
	}

	if (BattleState == ETTDBattleState::Running)
	{
		BattleElapsedSeconds += DeltaTime;
		UpdateBattlePhase();
		SpawnDueEnemies();
		CheckVictoryCondition();
		return;
	}

	if (BattleState == ETTDBattleState::Victory && BattlePhase == ETTDBattlePhase::VictoryDelay && !bPostBattleReturnReady)
	{
		BattleElapsedSeconds += DeltaTime;
		bPostBattleReturnReady = BattleElapsedSeconds >= VictoryReturnReadySeconds;
		if (bPostBattleReturnReady)
		{
			BroadcastPhaseChanged();
		}
	}
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
	return StartBattleInternal(LevelId, nullptr, OutFailureReason);
}

bool UTTDBattleWorldSubsystem::StartBattleWithLoadout(const FName LevelId, FTTDBattleLoadout Loadout, FText& OutFailureReason)
{
	return StartBattle(LevelId, Loadout, OutFailureReason);
}

bool UTTDBattleWorldSubsystem::StartBattle(const FName LevelId, const FTTDBattleLoadout& Loadout, FText& OutFailureReason)
{
	return StartBattleInternal(LevelId, &Loadout, OutFailureReason);
}

bool UTTDBattleWorldSubsystem::StartBattleInternal(const FName LevelId, const FTTDBattleLoadout* Loadout, FText& OutFailureReason)
{
	LoadDefinitionsFromSettings();
	GatherPlacedActors();

	const FTTDBattleLevelDefinition* LevelDefinition = LevelDefinitions.Find(LevelId);
	if (!LevelDefinition)
	{
		OutFailureReason = FText::Format(BattleText(TEXT("战斗关卡 {0} 未配置。")), FText::FromName(LevelId));
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
	BattlePhase = ETTDBattlePhase::None;
	BattleElapsedSeconds = 0.0f;
	Currency = FMath::Max(0, LevelDefinition->StartingCurrency);
	BuildingCapacity = FMath::Max(0, LevelDefinition->StartingBuildingCapacity);
	BuildingCapacityPurchaseCost = FMath::Max(0, LevelDefinition->BuildingCapacityPurchaseCost);
	BuildingLevelCap = FMath::Max(1, LevelDefinition->StartingBuildingLevelCap);
	BuildingLevelCapPurchaseCost = FMath::Max(0, LevelDefinition->BuildingLevelCapPurchaseCost);
	bAllowArbitraryBuildPlacement = LevelDefinition->bAllowArbitraryBuildPlacement;
	ArbitraryBuildMinSpacing = FMath::Max(0.0f, LevelDefinition->ArbitraryBuildMinSpacing);
	TotalEnemyWeight = 0.0f;
	RemainingUnspawnedWeight = 0.0f;
	AliveEnemyWeight = 0.0f;
	FrozenProgress = 0.0f;
	VictoryReturnDelaySeconds = FMath::Max(0.0f, LevelDefinition->VictoryReturnDelaySeconds);
	VictoryReturnReadySeconds = 0.0f;
	CurrentWaveIndex = INDEX_NONE;
	bUseFrozenProgress = false;
	bPostBattleReturnReady = false;
	bBroadcastedInitialState = false;

	const TArray<FName>& SourceDiagramIds = Loadout ? Loadout->SelectedDiagramIds : LevelDefinition->StartingDiagramIds;
	for (const FName DiagramId : SourceDiagramIds)
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

	const TArray<FTTDNameStack>& SourceToyBoxes = Loadout ? Loadout->SelectedToyBoxes : LevelDefinition->StartingToyBoxes;
	for (const FTTDNameStack& ToyBoxStack : SourceToyBoxes)
	{
		AddToyBoxCount(ToyBoxStack.Id, ToyBoxStack.Count);
	}

	if (!SpawnBackgroundForLevel(*LevelDefinition))
	{
		CleanupBattle();
		OutFailureReason = BattleText(TEXT("战斗背景生成失败。"));
		return false;
	}

	if (!SpawnCastleForLevel(*LevelDefinition))
	{
		CleanupBattle();
		OutFailureReason = BattleText(TEXT("城堡生成失败。"));
		return false;
	}

	BuildSpawnQueueForLevel(*LevelDefinition);
	BroadcastStateChanged();
	UpdateBattlePhase();
	BroadcastCurrencyChanged();
	BroadcastInventoryChanged();
	BroadcastCastleHealthChanged();
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
	switch (FinalState)
	{
	case ETTDBattleState::Victory:
		VictoryReturnReadySeconds = BattleElapsedSeconds + VictoryReturnDelaySeconds;
		SetBattlePhase(ETTDBattlePhase::VictoryDelay, CurrentWaveIndex);
		break;
	case ETTDBattleState::Defeat:
		SetBattlePhase(ETTDBattlePhase::Defeat, CurrentWaveIndex);
		break;
	case ETTDBattleState::ConfigurationError:
		SetBattlePhase(ETTDBattlePhase::ConfigurationError, CurrentWaveIndex);
		break;
	case ETTDBattleState::Inactive:
		SetBattlePhase(ETTDBattlePhase::None, INDEX_NONE);
		break;
	case ETTDBattleState::Running:
	default:
		break;
	}

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

void UTTDBattleWorldSubsystem::ClearEndedBattle()
{
	if (BattleState == ETTDBattleState::Running)
	{
		return;
	}

	CleanupBattle();
}

bool UTTDBattleWorldSubsystem::TryBuildOnSlot(const FName BuildingId, ATTDBuildSlotActor* Slot, FText& OutFailureReason)
{
	OutFailureReason = FText::GetEmpty();
	if (BattleState != ETTDBattleState::Running)
	{
		OutFailureReason = BattleText(TEXT("战斗尚未进行。"));
		return false;
	}

	if (!Slot)
	{
		OutFailureReason = BattleText(TEXT("建造位置无效。"));
		return false;
	}

	if (Slot->IsOccupied())
	{
		OutFailureReason = BattleText(TEXT("建造位置已被占用。"));
		return false;
	}

	if (!CanBuildBuilding(BuildingId, OutFailureReason))
	{
		return false;
	}

	const FTTDBuildingDefinition* BuildingDefinition = BuildingDefinitions.Find(BuildingId);
	check(BuildingDefinition);

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
		OutFailureReason = BattleText(TEXT("建筑生成失败。"));
		return false;
	}

	ApplyPartCosts(BuildingDefinition->PartCosts);
	const FTTDBattleBuildingRuntimeStats RuntimeStats = BuildRuntimeStatsForSlot(*BuildingDefinition, Slot);
	Building->InitializeBuilding(BuildingId, *BuildingDefinition, RuntimeStats, BuildingDefinition->PartCosts, Slot);
	Slot->SetOccupyingBuilding(Building);
	ActiveBuildings.Add(Building);
	RegisterBattleTarget(Building);

	BroadcastInventoryChanged();
	BroadcastBuildingPlaced(BuildingId, Slot->GetSlotId());
	return true;
}

bool UTTDBattleWorldSubsystem::TryBuildAtLocation(const FName BuildingId, FVector Location, FText& OutFailureReason)
{
	OutFailureReason = FText::GetEmpty();
	if (BattleState != ETTDBattleState::Running)
	{
		OutFailureReason = BattleText(TEXT("战斗尚未进行。"));
		return false;
	}

	if (!bAllowArbitraryBuildPlacement)
	{
		OutFailureReason = BattleText(TEXT("当前关卡未启用任意位置建造。"));
		return false;
	}

	if (!CanBuildBuilding(BuildingId, OutFailureReason))
	{
		return false;
	}

	if (!IsBuildLocationAvailable(Location, OutFailureReason))
	{
		return false;
	}

	Location.Z = 0.0f;
	ATTDBuildSlotActor* RuntimeSlot = CreateRuntimeBuildSlot(Location);
	if (!RuntimeSlot)
	{
		OutFailureReason = BattleText(TEXT("运行时建造位生成失败。"));
		return false;
	}

	if (!TryBuildOnSlot(BuildingId, RuntimeSlot, OutFailureReason))
	{
		BuildSlots.Remove(RuntimeSlot);
		RuntimeSlot->Destroy();
		return false;
	}

	return true;
}

bool UTTDBattleWorldSubsystem::BuyToyBox(const FName ToyBoxId, FText& OutFailureReason)
{
	OutFailureReason = FText::GetEmpty();
	const FName NormalizedToyBoxId = NormalizeToyBoxId(ToyBoxId);
	const FTTDToyBoxRewardDefinition* ToyBoxDefinition = ToyBoxRewardDefinitions.Find(NormalizedToyBoxId);
	if (!ToyBoxDefinition)
	{
		OutFailureReason = BattleText(TEXT("玩具盒未配置。"));
		return false;
	}

	if (Currency < ToyBoxDefinition->PurchaseCost)
	{
		OutFailureReason = BattleText(TEXT("货币不足。"));
		return false;
	}

	Currency -= ToyBoxDefinition->PurchaseCost;
	AddToyBoxCount(NormalizedToyBoxId, 1);
	BroadcastCurrencyChanged();
	BroadcastInventoryChanged();
	return true;
}

bool UTTDBattleWorldSubsystem::BuyBuildingCapacity(FText& OutFailureReason)
{
	OutFailureReason = FText::GetEmpty();
	if (BattleState != ETTDBattleState::Running)
	{
		OutFailureReason = BattleText(TEXT("战斗尚未进行。"));
		return false;
	}

	if (BuildingCapacity <= 0)
	{
		OutFailureReason = BattleText(TEXT("当前关卡未启用建筑数量上限。"));
		return false;
	}

	if (Currency < BuildingCapacityPurchaseCost)
	{
		OutFailureReason = BattleText(TEXT("货币不足。"));
		return false;
	}

	Currency -= BuildingCapacityPurchaseCost;
	++BuildingCapacity;
	BroadcastCurrencyChanged();
	BroadcastInventoryChanged();
	return true;
}

bool UTTDBattleWorldSubsystem::BuyBuildingLevelCap(FText& OutFailureReason)
{
	OutFailureReason = FText::GetEmpty();
	if (BattleState != ETTDBattleState::Running)
	{
		OutFailureReason = BattleText(TEXT("战斗尚未进行。"));
		return false;
	}

	if (BuildingLevelCapPurchaseCost <= 0)
	{
		OutFailureReason = BattleText(TEXT("当前关卡未配置等级上限商品。"));
		return false;
	}

	if (Currency < BuildingLevelCapPurchaseCost)
	{
		OutFailureReason = BattleText(TEXT("货币不足。"));
		return false;
	}

	Currency -= BuildingLevelCapPurchaseCost;
	++BuildingLevelCap;
	BroadcastCurrencyChanged();
	BroadcastInventoryChanged();
	return true;
}

bool UTTDBattleWorldSubsystem::CanUpgradeBuilding(ATTDBattleBuildingActor* Building, const FName PartId, FText& OutFailureReason) const
{
	if (BattleState != ETTDBattleState::Running)
	{
		OutFailureReason = BattleText(TEXT("战斗尚未进行。"));
		return false;
	}

	if (!IsActiveBuilding(Building))
	{
		OutFailureReason = BattleText(TEXT("建筑无效。"));
		return false;
	}

	if (Building->GetBuildingLevel() >= BuildingLevelCap)
	{
		OutFailureReason = BattleText(TEXT("建筑已达到当前等级上限。"));
		return false;
	}

	const FTTDBuildingDefinition* BuildingDefinition = BuildingDefinitions.Find(Building->GetBuildingId());
	if (!BuildingDefinition)
	{
		OutFailureReason = BattleText(TEXT("建筑未配置。"));
		return false;
	}

	const FTTDBattleBuildingUpgradeOption* UpgradeOption = FindBuildingUpgradeOption(*BuildingDefinition, PartId);
	if (!UpgradeOption)
	{
		OutFailureReason = BattleText(TEXT("该零件不能升级这个建筑。"));
		return false;
	}

	if (!HasPartCount(UpgradeOption->PartId, FMath::Max(1, UpgradeOption->PartCost)))
	{
		OutFailureReason = BattleText(TEXT("升级零件不足。"));
		return false;
	}

	OutFailureReason = FText::GetEmpty();
	return true;
}

bool UTTDBattleWorldSubsystem::UpgradeBuilding(ATTDBattleBuildingActor* Building, const FName PartId, FText& OutFailureReason)
{
	if (!CanUpgradeBuilding(Building, PartId, OutFailureReason))
	{
		return false;
	}

	const FTTDBuildingDefinition* BuildingDefinition = BuildingDefinitions.Find(Building->GetBuildingId());
	check(BuildingDefinition);
	const FTTDBattleBuildingUpgradeOption* UpgradeOption = FindBuildingUpgradeOption(*BuildingDefinition, PartId);
	check(UpgradeOption);

	const int32 PartCost = FMath::Max(1, UpgradeOption->PartCost);
	TArray<FTTDNameStack> UpgradeCost;
	UpgradeCost.Add(FTTDNameStack(UpgradeOption->PartId, PartCost));
	ApplyPartCosts(UpgradeCost);
	FTTDBattleBuildingRuntimeStats RuntimeStats = ApplyUpgradeModifiers(Building->GetRuntimeStats(), UpgradeOption->Modifiers);
	NormalizeRuntimeStats(RuntimeStats);
	Building->ApplyRuntimeStats(RuntimeStats);
	Building->IncrementLevel();
	Building->AddUpgradePartCost(UpgradeOption->PartId, PartCost);
	BroadcastInventoryChanged();
	return true;
}

bool UTTDBattleWorldSubsystem::SellBuilding(ATTDBattleBuildingActor* Building, FText& OutFailureReason)
{
	OutFailureReason = FText::GetEmpty();
	if (BattleState != ETTDBattleState::Running)
	{
		OutFailureReason = BattleText(TEXT("战斗尚未进行。"));
		return false;
	}

	if (!IsActiveBuilding(Building))
	{
		OutFailureReason = BattleText(TEXT("建筑无效。"));
		return false;
	}

	if (ATTDBuildSlotActor* Slot = Building->GetOwningSlot())
	{
		Slot->ClearOccupyingBuilding(Building);
	}

	for (const FTTDNameStack& ConstructionCost : Building->GetConstructionPartCosts())
	{
		AddPartCount(ConstructionCost.Id, ConstructionCost.Count);
	}

	ActiveBuildings.Remove(Building);
	UnregisterBattleTarget(Building);
	ReleaseBattleActor(Building);
	BroadcastInventoryChanged();
	return true;
}

bool UTTDBattleWorldSubsystem::OpenToyBox(FName ToyBoxId, TArray<FTTDNameStack>& OutRewards, FText& OutFailureReason)
{
	OutRewards.Reset();
	OutFailureReason = FText::GetEmpty();

	const FName NormalizedToyBoxId = NormalizeToyBoxId(ToyBoxId);
	const FTTDToyBoxRewardDefinition* ToyBoxDefinition = ToyBoxRewardDefinitions.Find(NormalizedToyBoxId);
	if (!ToyBoxDefinition)
	{
		OutFailureReason = BattleText(TEXT("玩具盒未配置。"));
		return false;
	}

	int32* ToyBoxCount = ToyBoxCounts.Find(NormalizedToyBoxId);
	if (!ToyBoxCount || *ToyBoxCount <= 0)
	{
		OutFailureReason = BattleText(TEXT("没有可打开的玩具盒。"));
		return false;
	}

	float TotalWeight = 0.0f;
	for (const FTTDToyBoxRewardEntry& Entry : ToyBoxDefinition->RewardEntries)
	{
		TotalWeight += FMath::Max(0.0f, Entry.Weight);
	}

	if (TotalWeight <= 0.0f)
	{
		OutFailureReason = BattleText(TEXT("玩具盒没有有效奖励。"));
		return false;
	}

	--(*ToyBoxCount);
	if (*ToyBoxCount <= 0)
	{
		ToyBoxCounts.Remove(NormalizedToyBoxId);
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

bool UTTDBattleWorldSubsystem::ApplyTestCheatSupplies(FText& OutSummary)
{
	if (BattleState != ETTDBattleState::Running)
	{
		OutSummary = BattleText(TEXT("战斗尚未进行。"));
		return false;
	}

	int32 AddedDiagramCount = 0;
	int32 AddedPartStackCount = 0;
	int32 AddedToyBoxStackCount = 0;

	for (const TPair<FName, FTTDBuildingDefinition>& Pair : BuildingDefinitions)
	{
		const FTTDBuildingDefinition& BuildingDefinition = Pair.Value;
		if (!BuildingDefinition.RequiredDiagramId.IsNone() && !DiagramIds.Contains(BuildingDefinition.RequiredDiagramId))
		{
			DiagramIds.Add(BuildingDefinition.RequiredDiagramId);
			++AddedDiagramCount;
		}

		for (const FTTDNameStack& PartCost : BuildingDefinition.PartCosts)
		{
			if (PartCost.Id.IsNone())
			{
				continue;
			}

			const int32 Quantity = FMath::Max(10, FMath::Max(0, PartCost.Count) * 10);
			AddPartCount(PartCost.Id, Quantity);
			++AddedPartStackCount;
		}

		for (const FTTDBattleBuildingUpgradeOption& UpgradeOption : BuildingDefinition.UpgradeOptions)
		{
			if (UpgradeOption.PartId.IsNone())
			{
				continue;
			}

			AddPartCount(UpgradeOption.PartId, FMath::Max(10, FMath::Max(1, UpgradeOption.PartCost) * 10));
			++AddedPartStackCount;
		}
	}

	for (const TPair<FName, FTTDToyBoxRewardDefinition>& Pair : ToyBoxRewardDefinitions)
	{
		if (!Pair.Key.IsNone())
		{
			AddToyBoxCount(Pair.Key, 3);
			++AddedToyBoxStackCount;
		}
	}

	Currency = FMath::Max(Currency, 999);
	BroadcastCurrencyChanged();
	BroadcastInventoryChanged();

	OutSummary = FText::Format(
		BattleText(TEXT("测试补给已发放：货币 999、图纸 +{0}、零件项 +{1}、玩具盒项 +{2}。")),
		FText::AsNumber(AddedDiagramCount),
		FText::AsNumber(AddedPartStackCount),
		FText::AsNumber(AddedToyBoxStackCount));
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

TArray<FTTDBuildingDefinition> UTTDBattleWorldSubsystem::GetAvailableBuildingDefinitions() const
{
	TArray<FTTDBuildingDefinition> Result;
	for (const TPair<FName, FTTDBuildingDefinition>& Pair : BuildingDefinitions)
	{
		const FTTDBuildingDefinition& BuildingDefinition = Pair.Value;
		if (BuildingDefinition.RequiredDiagramId.IsNone() || DiagramIds.Contains(BuildingDefinition.RequiredDiagramId))
		{
			Result.Add(BuildingDefinition);
		}
	}

	Result.Sort(
		[](const FTTDBuildingDefinition& Left, const FTTDBuildingDefinition& Right)
		{
			return Left.BuildingId.LexicalLess(Right.BuildingId);
		});
	return Result;
}

bool UTTDBattleWorldSubsystem::IsBuildingAvailable(const FName BuildingId) const
{
	const FTTDBuildingDefinition* BuildingDefinition = BuildingDefinitions.Find(BuildingId);
	return BuildingDefinition
		&& (BuildingDefinition->RequiredDiagramId.IsNone() || DiagramIds.Contains(BuildingDefinition->RequiredDiagramId));
}

bool UTTDBattleWorldSubsystem::CanBuildBuilding(const FName BuildingId, FText& OutFailureReason) const
{
	if (BattleState != ETTDBattleState::Running)
	{
		OutFailureReason = BattleText(TEXT("战斗尚未进行。"));
		return false;
	}

	if (BuildingCapacity > 0 && ActiveBuildings.Num() >= BuildingCapacity)
	{
		OutFailureReason = BattleText(TEXT("建筑数量已达上限。"));
		return false;
	}

	const FTTDBuildingDefinition* BuildingDefinition = BuildingDefinitions.Find(BuildingId);
	if (!BuildingDefinition)
	{
		OutFailureReason = BattleText(TEXT("建筑未配置。"));
		return false;
	}

	if (!BuildingDefinition->RequiredDiagramId.IsNone() && !DiagramIds.Contains(BuildingDefinition->RequiredDiagramId))
	{
		OutFailureReason = BattleText(TEXT("缺少所需图纸。"));
		return false;
	}

	if (!HasPartCosts(BuildingDefinition->PartCosts))
	{
		OutFailureReason = BattleText(TEXT("零件不足。"));
		return false;
	}

	OutFailureReason = FText::GetEmpty();
	return true;
}

bool UTTDBattleWorldSubsystem::IsActiveBuilding(ATTDBattleBuildingActor* Building) const
{
	return IsValid(Building) && ActiveBuildings.Contains(Building);
}

TArray<FTTDToyBoxRewardDefinition> UTTDBattleWorldSubsystem::GetToyBoxDefinitions() const
{
	TArray<FTTDToyBoxRewardDefinition> Result;
	ToyBoxRewardDefinitions.GenerateValueArray(Result);
	return Result;
}

TArray<FTTDBattleLevelDefinition> UTTDBattleWorldSubsystem::GetBattleLevelDefinitions() const
{
	TArray<FTTDBattleLevelDefinition> Result;
	LevelDefinitions.GenerateValueArray(Result);
	Result.Sort(
		[](const FTTDBattleLevelDefinition& Left, const FTTDBattleLevelDefinition& Right)
		{
			return Left.LevelId.LexicalLess(Right.LevelId);
		});
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

float UTTDBattleWorldSubsystem::GetWaveRemainingSeconds() const
{
	if (CurrentWaveIndex < 0 || !WaveRuntimes.IsValidIndex(CurrentWaveIndex))
	{
		return 0.0f;
	}

	const FTTDBattleWaveRuntime& WaveRuntime = WaveRuntimes[CurrentWaveIndex];
	if (BattlePhase != ETTDBattlePhase::WaveActive)
	{
		return 0.0f;
	}

	return FMath::Max(0.0f, WaveRuntime.WaveEndSeconds - BattleElapsedSeconds);
}

float UTTDBattleWorldSubsystem::GetPhaseRemainingSeconds() const
{
	if (CurrentWaveIndex >= 0 && WaveRuntimes.IsValidIndex(CurrentWaveIndex))
	{
		const FTTDBattleWaveRuntime& WaveRuntime = WaveRuntimes[CurrentWaveIndex];
		if (BattlePhase == ETTDBattlePhase::Preparing)
		{
			return FMath::Max(0.0f, WaveRuntime.BufferStartSeconds - BattleElapsedSeconds);
		}

		if (BattlePhase == ETTDBattlePhase::Buffer)
		{
			return FMath::Max(0.0f, WaveRuntime.WaveStartSeconds - BattleElapsedSeconds);
		}

		if (BattlePhase == ETTDBattlePhase::WaveActive)
		{
			return FMath::Max(0.0f, WaveRuntime.WaveEndSeconds - BattleElapsedSeconds);
		}
	}

	if (BattlePhase == ETTDBattlePhase::VictoryDelay)
	{
		return FMath::Max(0.0f, VictoryReturnReadySeconds - BattleElapsedSeconds);
	}

	return 0.0f;
}

void UTTDBattleWorldSubsystem::HandleCastleDestroyed(ATTDBattleCastleActor* DestroyedCastle)
{
	if (DestroyedCastle && DestroyedCastle == CastleActor)
	{
		EndBattle(ETTDBattleState::Defeat);
	}
}

void UTTDBattleWorldSubsystem::HandleCastleDamaged(ATTDBattleCastleActor* DamagedCastle)
{
	if (DamagedCastle && DamagedCastle == CastleActor)
	{
		BroadcastCastleHealthChanged();
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

	BroadcastInventoryChanged();
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
	if (Enemy->GetCurrencyDrop() > 0 && Enemy->GetCurrencyDropChance() > 0.0f && RewardRandomStream.FRand() <= Enemy->GetCurrencyDropChance())
	{
		Currency += Enemy->GetCurrencyDrop();
	}

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
	return ToyBoxRewardDefinitions.Find(NormalizeToyBoxId(ToyBoxId));
}

FTTDBattleBuildingRuntimeStats UTTDBattleWorldSubsystem::BuildRuntimeStatsForSlot(const FTTDBuildingDefinition& BuildingDefinition, const ATTDBuildSlotActor* Slot) const
{
	FTTDBattleBuildingRuntimeStats RuntimeStats = BuildingDefinition.BaseStats;
	if (!Slot || Slot->GetHeightEffectId().IsNone())
	{
		NormalizeRuntimeStats(RuntimeStats);
		return RuntimeStats;
	}

	const FTTDBattleHeightEffectDefinition* HeightEffect = HeightEffectDefinitions.Find(Slot->GetHeightEffectId());
	if (!HeightEffect)
	{
		NormalizeRuntimeStats(RuntimeStats);
		return RuntimeStats;
	}

	RuntimeStats = ApplyUpgradeModifiers(RuntimeStats, HeightEffect->Modifiers);
	NormalizeRuntimeStats(RuntimeStats);
	return RuntimeStats;
}

FTTDBattleBuildingRuntimeStats UTTDBattleWorldSubsystem::ApplyUpgradeModifiers(FTTDBattleBuildingRuntimeStats RuntimeStats, const TArray<FTTDBattleAttributeModifier>& Modifiers) const
{
	for (const FTTDBattleAttributeModifier& Modifier : Modifiers)
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
		case ETTDBattleAttribute::MoveSpeed:
			ApplyModifier(RuntimeStats.MoveSpeed, Modifier);
			break;
		default:
			break;
		}
	}

	NormalizeRuntimeStats(RuntimeStats);
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
	if (FTTDBuildingDefinition* ProjectileTowerDefinition = BuildingDefinitions.Find(TEXT("ProjectileTower")))
	{
		if (ProjectileTowerDefinition->RequiredDiagramId.IsNone() || ProjectileTowerDefinition->RequiredDiagramId == TEXT("BasicTower"))
		{
			ProjectileTowerDefinition->RequiredDiagramId = TEXT("ProjectileTower");
		}
	}

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
	NormalizeLoadedToyBoxDefinitions();

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

void UTTDBattleWorldSubsystem::NormalizeLoadedToyBoxDefinitions()
{
	if (FTTDToyBoxRewardDefinition* BasicToyBoxDefinition = ToyBoxRewardDefinitions.Find(BasicToyBoxId))
	{
		if (BasicToyBoxDefinition->ToyBoxId == LegacyBasicToyBoxId || BasicToyBoxDefinition->ToyBoxId.IsNone())
		{
			BasicToyBoxDefinition->ToyBoxId = BasicToyBoxId;
		}
		ToyBoxRewardDefinitions.Remove(LegacyBasicToyBoxId);
		return;
	}

	FTTDToyBoxRewardDefinition LegacyDefinition;
	if (!ToyBoxRewardDefinitions.RemoveAndCopyValue(LegacyBasicToyBoxId, LegacyDefinition))
	{
		return;
	}

	LegacyDefinition.ToyBoxId = BasicToyBoxId;
	ToyBoxRewardDefinitions.Add(BasicToyBoxId, LegacyDefinition);
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
		if (!It->IsRuntimeGeneratedSlot())
		{
			BuildSlots.Add(*It);
		}
	}
}

bool UTTDBattleWorldSubsystem::ValidateBattleStart(const FName LevelId, FText& OutFailureReason) const
{
	if (LevelId.IsNone())
	{
		OutFailureReason = BattleText(TEXT("默认战斗关卡 ID 为空。"));
		return false;
	}

	const FTTDBattleLevelDefinition* LevelDefinition = LevelDefinitions.Find(LevelId);
	if (!LevelDefinition)
	{
		OutFailureReason = FText::Format(BattleText(TEXT("战斗关卡 {0} 未配置。")), FText::FromName(LevelId));
		return false;
	}

	return ValidateLevelDefinition(LevelId, *LevelDefinition, OutFailureReason)
		&& ValidateProjectileBuildingDefinitions(OutFailureReason);
}

bool UTTDBattleWorldSubsystem::ValidateLevelDefinition(const FName LevelId, const FTTDBattleLevelDefinition& LevelDefinition, FText& OutFailureReason) const
{
	if (SpawnPoints.IsEmpty())
	{
		OutFailureReason = BattleText(TEXT("战斗关卡需要场景中至少有一个刷怪点。"));
		return false;
	}

	if (BuildSlots.IsEmpty() && !LevelDefinition.bAllowArbitraryBuildPlacement)
	{
		OutFailureReason = BattleText(TEXT("战斗关卡需要场景中至少有一个建造位。"));
		return false;
	}

	UClass* CastleClass = LevelDefinition.CastleClass.LoadSynchronous();
	if (CastleClass && !CastleClass->IsChildOf(ATTDBattleCastleActor::StaticClass()))
	{
		OutFailureReason = FText::Format(BattleText(TEXT("关卡 {0} 的城堡类必须继承 ATTDBattleCastleActor。")), FText::FromName(LevelId));
		return false;
	}

	UClass* BackgroundClass = LevelDefinition.Background.bSpawnBackground
		? LevelDefinition.Background.BackgroundClass.LoadSynchronous()
		: nullptr;
	if (BackgroundClass && !BackgroundClass->IsChildOf(ATTDBattleBackgroundActor::StaticClass()))
	{
		OutFailureReason = FText::Format(BattleText(TEXT("关卡 {0} 的背景类必须继承 ATTDBattleBackgroundActor。")), FText::FromName(LevelId));
		return false;
	}

	for (const FName WaveId : LevelDefinition.WaveIds)
	{
		const FTTDWaveDefinition* WaveDefinition = WaveDefinitions.Find(WaveId);
		if (!WaveDefinition)
		{
			OutFailureReason = FText::Format(BattleText(TEXT("战斗关卡 {0} 引用了未配置的波次 {1}。")), FText::FromName(LevelId), FText::FromName(WaveId));
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
					? BattleText(TEXT("波次需要刷怪点，但当前没有可用刷怪点。"))
					: FText::Format(BattleText(TEXT("波次需要刷怪组 {0}，但场景中没有匹配刷怪点。")), FText::FromName(EnemyEntry.SpawnGroup));
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
		OutFailureReason = FText::Format(BattleText(TEXT("敌人 {0} 未配置。")), FText::FromName(EnemyId));
		return false;
	}

	UClass* EnemyClass = ResolveEnemyClass(*EnemyDefinition);
	if (!IsPoolableActorClass(EnemyClass, ATTDBattleEnemyActor::StaticClass()))
	{
		OutFailureReason = FText::Format(BattleText(TEXT("敌人 {0} 的类必须继承 ATTDBattleEnemyActor 并实现池化接口。")), FText::FromName(EnemyId));
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
			OutFailureReason = FText::Format(BattleText(TEXT("建筑 {0} 的投射物类必须继承 ATTDBattleProjectileActor 并实现池化接口。")), FText::FromName(BuildingDefinition.BuildingId));
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
	if (IsValid(Cast<AActor>(ActiveBackgroundActor)))
	{
		Cast<AActor>(ActiveBackgroundActor)->Destroy();
	}

	ActiveBackgroundActor = nullptr;
	for (ATTDBuildSlotActor* Slot : BuildSlots)
	{
		if (IsValid(Slot) && Slot->IsRuntimeGeneratedSlot())
		{
			Slot->Destroy();
		}
	}

	SpawnPoints.Reset();
	BuildSlots.Reset();
	BattleTargets.Reset();
	PendingSpawns.Reset();
	WaveRuntimes.Reset();
	DiagramIds.Reset();
	PartCounts.Reset();
	ToyBoxCounts.Reset();
	ActiveLevelId = NAME_None;
	BattleState = ETTDBattleState::Inactive;
	BattlePhase = ETTDBattlePhase::None;
	BattleElapsedSeconds = 0.0f;
	TotalEnemyWeight = 0.0f;
	RemainingUnspawnedWeight = 0.0f;
	AliveEnemyWeight = 0.0f;
	FrozenProgress = 0.0f;
	VictoryReturnReadySeconds = 0.0f;
	Currency = 0;
	BuildingCapacity = 0;
	BuildingCapacityPurchaseCost = 0;
	BuildingLevelCap = 1;
	BuildingLevelCapPurchaseCost = 0;
	bAllowArbitraryBuildPlacement = true;
	ArbitraryBuildMinSpacing = 180.0f;
	CurrentWaveIndex = INDEX_NONE;
	bUseFrozenProgress = false;
	bPostBattleReturnReady = false;
}

bool UTTDBattleWorldSubsystem::SpawnBackgroundForLevel(const FTTDBattleLevelDefinition& LevelDefinition)
{
	if (!LevelDefinition.Background.bSpawnBackground)
	{
		return true;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	UClass* BackgroundClass = LevelDefinition.Background.BackgroundClass.LoadSynchronous();
	if (!BackgroundClass)
	{
		BackgroundClass = ATTDBattleBackgroundActor::StaticClass();
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ActiveBackgroundActor = World->SpawnActor<ATTDBattleBackgroundActor>(BackgroundClass, LevelDefinition.Background.Transform, SpawnParameters);
	if (!ActiveBackgroundActor)
	{
		UE_LOG(LogTTDBattle, Warning, TEXT("StartBattle failed: background class %s did not spawn a battle background."), *GetNameSafe(BackgroundClass));
		return false;
	}

	ActiveBackgroundActor->ConfigureBackground(LevelDefinition.Background);
	return true;
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
	WaveRuntimes.Reset();
	float TimelineCursor = FMath::Max(0.0f, LevelDefinition.PreparationDurationSeconds);

	for (const FName WaveId : LevelDefinition.WaveIds)
	{
		const FTTDWaveDefinition* WaveDefinition = WaveDefinitions.Find(WaveId);
		if (!WaveDefinition)
		{
			UE_LOG(LogTTDBattle, Warning, TEXT("Battle level %s references missing wave %s."), *LevelDefinition.LevelId.ToString(), *WaveId.ToString());
			continue;
		}

		FTTDBattleWaveRuntime WaveRuntime;
		WaveRuntime.WaveId = WaveId;
		WaveRuntime.BufferStartSeconds = TimelineCursor;
		TimelineCursor += FMath::Max(0.0f, WaveDefinition->DelayBeforeWave);
		WaveRuntime.WaveStartSeconds = TimelineCursor;

		float LastSpawnOffset = 0.0f;
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
				PendingSpawn.SpawnTimeSeconds = WaveRuntime.WaveStartSeconds + SpawnIndex * EntryInterval;
				PendingSpawn.ProgressWeight = FMath::Max(0.0f, EnemyDefinition->ProgressWeight);
				PendingSpawns.Add(PendingSpawn);
				TotalEnemyWeight += PendingSpawn.ProgressWeight;
				RemainingUnspawnedWeight += PendingSpawn.ProgressWeight;
				LastSpawnOffset = FMath::Max(LastSpawnOffset, SpawnIndex * EntryInterval);
			}
		}

		const float ConfiguredDuration = FMath::Max(0.0f, WaveDefinition->WaveDurationSeconds);
		const float WaveDuration = ConfiguredDuration > 0.0f ? ConfiguredDuration : LastSpawnOffset;
		WaveRuntime.WaveEndSeconds = WaveRuntime.WaveStartSeconds + WaveDuration;
		TimelineCursor = WaveRuntime.WaveEndSeconds;
		WaveRuntimes.Add(WaveRuntime);
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
		OutFailureReason = FText::Format(BattleText(TEXT("敌人 {0} 未配置。")), FText::FromName(PendingSpawn.EnemyId));
		return false;
	}

	UClass* EnemyClass = ResolveEnemyClass(*EnemyDefinition);
	if (!IsPoolableActorClass(EnemyClass, ATTDBattleEnemyActor::StaticClass()))
	{
		OutFailureReason = FText::Format(BattleText(TEXT("敌人 {0} 的类无效。")), FText::FromName(PendingSpawn.EnemyId));
		return false;
	}

	const ATTDBattleSpawnPointActor* SpawnPoint = PickSpawnPoint(PendingSpawn.SpawnGroup);
	if (!SpawnPoint)
	{
		OutFailureReason = PendingSpawn.SpawnGroup.IsNone()
			? BattleText(TEXT("没有可用刷怪点。"))
			: FText::Format(BattleText(TEXT("没有找到刷怪组 {0} 的刷怪点。")), FText::FromName(PendingSpawn.SpawnGroup));
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
		OutFailureReason = FText::Format(BattleText(TEXT("敌人类 {0} 无法从对象池获取。")), FText::FromString(GetNameSafe(EnemyClass)));
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

void UTTDBattleWorldSubsystem::UpdateBattlePhase()
{
	if (BattleState != ETTDBattleState::Running)
	{
		return;
	}

	if (WaveRuntimes.IsEmpty())
	{
		SetBattlePhase(ETTDBattlePhase::WaveActive, INDEX_NONE);
		return;
	}

	int32 NewWaveIndex = WaveRuntimes.Num() - 1;
	ETTDBattlePhase NewPhase = ETTDBattlePhase::WaveActive;
	for (int32 Index = 0; Index < WaveRuntimes.Num(); ++Index)
	{
		const FTTDBattleWaveRuntime& WaveRuntime = WaveRuntimes[Index];
		if (BattleElapsedSeconds < WaveRuntime.WaveStartSeconds)
		{
			NewWaveIndex = Index;
			NewPhase = Index == 0 && BattleElapsedSeconds < WaveRuntime.BufferStartSeconds
				? ETTDBattlePhase::Preparing
				: ETTDBattlePhase::Buffer;
			break;
		}

		if (BattleElapsedSeconds <= WaveRuntime.WaveEndSeconds)
		{
			NewWaveIndex = Index;
			NewPhase = ETTDBattlePhase::WaveActive;
			break;
		}
	}

	SetBattlePhase(NewPhase, NewWaveIndex);
}

void UTTDBattleWorldSubsystem::SetBattlePhase(const ETTDBattlePhase NewPhase, const int32 NewWaveIndex)
{
	if (BattlePhase == NewPhase && CurrentWaveIndex == NewWaveIndex)
	{
		return;
	}

	BattlePhase = NewPhase;
	CurrentWaveIndex = NewWaveIndex;
	BroadcastPhaseChanged();
}

void UTTDBattleWorldSubsystem::CheckVictoryCondition()
{
	const bool bAllWaveTimersExpired = WaveRuntimes.IsEmpty()
		|| BattleElapsedSeconds >= WaveRuntimes.Last().WaveEndSeconds;
	if (BattleState == ETTDBattleState::Running && bAllWaveTimersExpired && PendingSpawns.IsEmpty() && ActiveEnemies.IsEmpty())
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
	const FName NormalizedToyBoxId = NormalizeToyBoxId(ToyBoxId);
	if (!NormalizedToyBoxId.IsNone() && Count > 0)
	{
		ToyBoxCounts.FindOrAdd(NormalizedToyBoxId) += Count;
	}
}

bool UTTDBattleWorldSubsystem::IsBuildLocationAvailable(const FVector Location, FText& OutFailureReason) const
{
	if (ActiveBackgroundActor)
	{
		const FTTDBattleBackgroundDefinition& BackgroundDefinition = ActiveBackgroundActor->GetBackgroundDefinition();
		const FVector BackgroundLocation = ActiveBackgroundActor->GetActorLocation();
		const FVector2D HalfExtent(
			FMath::Max(100.0f, BackgroundDefinition.ArenaHalfExtent.X),
			FMath::Max(100.0f, BackgroundDefinition.ArenaHalfExtent.Y));
		if (Location.X < BackgroundLocation.X - HalfExtent.X
			|| Location.X > BackgroundLocation.X + HalfExtent.X
			|| Location.Y < BackgroundLocation.Y - HalfExtent.Y
			|| Location.Y > BackgroundLocation.Y + HalfExtent.Y)
		{
			OutFailureReason = BattleText(TEXT("建造位置超出战场范围。"));
			return false;
		}
	}

	const float MinSpacingSquared = FMath::Square(FMath::Max(0.0f, ArbitraryBuildMinSpacing));
	for (const ATTDBuildSlotActor* Slot : BuildSlots)
	{
		if (!IsValid(Slot) || !Slot->IsOccupied())
		{
			continue;
		}

		if (FVector::DistSquared2D(Location, Slot->GetActorLocation()) < MinSpacingSquared)
		{
			OutFailureReason = BattleText(TEXT("建造位置离已有建筑太近。"));
			return false;
		}
	}

	for (const ATTDBattleBuildingActor* Building : ActiveBuildings)
	{
		if (IsValid(Building) && FVector::DistSquared2D(Location, Building->GetActorLocation()) < MinSpacingSquared)
		{
			OutFailureReason = BattleText(TEXT("建造位置离已有建筑太近。"));
			return false;
		}
	}

	OutFailureReason = FText::GetEmpty();
	return true;
}

ATTDBuildSlotActor* UTTDBattleWorldSubsystem::CreateRuntimeBuildSlot(FVector Location)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	Location.Z = 0.0f;
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ATTDBuildSlotActor* RuntimeSlot = World->SpawnActor<ATTDBuildSlotActor>(ATTDBuildSlotActor::StaticClass(), Location, FRotator::ZeroRotator, SpawnParameters);
	if (!RuntimeSlot)
	{
		return nullptr;
	}

	RuntimeSlot->ConfigureSlot(
		FName(*FString::Printf(TEXT("RuntimeSlot_%d"), BuildSlots.Num())),
		FIntPoint(FMath::RoundToInt(Location.X), FMath::RoundToInt(Location.Y)),
		0,
		NAME_None);
	RuntimeSlot->MarkRuntimeGeneratedSlot();
	BuildSlots.Add(RuntimeSlot);
	return RuntimeSlot;
}

const FTTDBattleBuildingUpgradeOption* UTTDBattleWorldSubsystem::FindBuildingUpgradeOption(const FTTDBuildingDefinition& BuildingDefinition, const FName PartId) const
{
	for (const FTTDBattleBuildingUpgradeOption& UpgradeOption : BuildingDefinition.UpgradeOptions)
	{
		if (!UpgradeOption.PartId.IsNone() && UpgradeOption.PartId == PartId)
		{
			return &UpgradeOption;
		}
	}

	return nullptr;
}

bool UTTDBattleWorldSubsystem::HasPartCount(const FName PartId, const int32 Count) const
{
	return !PartId.IsNone() && Count > 0 && PartCounts.FindRef(PartId) >= Count;
}

void UTTDBattleWorldSubsystem::NormalizeRuntimeStats(FTTDBattleBuildingRuntimeStats& RuntimeStats) const
{
	RuntimeStats.MaxHealth = FMath::Max(1.0f, RuntimeStats.MaxHealth);
	RuntimeStats.AttackDamage = FMath::Max(0.0f, RuntimeStats.AttackDamage);
	RuntimeStats.AttackRange = FMath::Max(0.0f, RuntimeStats.AttackRange);
	RuntimeStats.AttackInterval = FMath::Max(0.01f, RuntimeStats.AttackInterval);
	RuntimeStats.ProjectileSpeed = FMath::Max(1.0f, RuntimeStats.ProjectileSpeed);
	RuntimeStats.MoveSpeed = FMath::Max(0.0f, RuntimeStats.MoveSpeed);
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

void UTTDBattleWorldSubsystem::BroadcastCastleHealthChanged() const
{
	if (const UWorld* World = GetWorld())
	{
		if (const UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (UGameplayMessageSubsystem* MessageSubsystem = GameInstance->GetSubsystem<UGameplayMessageSubsystem>())
			{
				FTTDBattleCastleHealthChangedMessage Message;
				Message.CurrentHealth = GetCastleCurrentHealth();
				Message.MaxHealth = GetCastleMaxHealth();
				MessageSubsystem->BroadcastMessage(TAG_TTD_Message_Battle_CastleHealth_Changed, Message);
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

void UTTDBattleWorldSubsystem::BroadcastPhaseChanged() const
{
	if (const UWorld* World = GetWorld())
	{
		if (const UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (UGameplayMessageSubsystem* MessageSubsystem = GameInstance->GetSubsystem<UGameplayMessageSubsystem>())
			{
				FTTDBattlePhaseChangedMessage Message;
				Message.LevelId = ActiveLevelId;
				Message.Phase = BattlePhase;
				Message.CurrentWaveNumber = GetCurrentWaveNumber();
				Message.TotalWaveCount = GetTotalWaveCount();
				Message.RemainingSeconds = GetPhaseRemainingSeconds();
				MessageSubsystem->BroadcastMessage(TAG_TTD_Message_Battle_Phase_Changed, Message);
			}
		}
	}
}
