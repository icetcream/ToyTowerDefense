#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "TTDBattleTypes.h"
#include "TTDBattleWorldSubsystem.generated.h"

class ATTDBattleBuildingActor;
class ATTDBattleCastleActor;
class ATTDBattleEnemyActor;
class ATTDBattleProjectileActor;
class ATTDBattleSpawnPointActor;
class ATTDBuildSlotActor;
class UTTDDeveloperSettings;

USTRUCT()
struct FTTDBattlePendingSpawn
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	FName EnemyId;

	UPROPERTY(Transient)
	FName SpawnGroup;

	UPROPERTY(Transient)
	float SpawnTimeSeconds = 0.0f;

	UPROPERTY(Transient)
	float ProgressWeight = 0.0f;
};

UCLASS(BlueprintType)
class TOYTOWERDEFENSE_API UTTDBattleWorldSubsystem : public UWorldSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override;
	virtual UWorld* GetTickableGameObjectWorld() const override;

	UFUNCTION(BlueprintCallable, Category = "Toy Tower Defense|Battle")
	bool StartDefaultBattle();

	bool StartDefaultBattle(FText& OutFailureReason);

	UFUNCTION(BlueprintCallable, Category = "Toy Tower Defense|Battle", meta = (DisplayName = "Start Default Battle With Reason"))
	bool StartDefaultBattleWithReason(FText& OutFailureReason);

	UFUNCTION(BlueprintCallable, Category = "Toy Tower Defense|Battle")
	bool StartBattle(FName LevelId);

	bool StartBattle(FName LevelId, FText& OutFailureReason);

	UFUNCTION(BlueprintCallable, Category = "Toy Tower Defense|Battle", meta = (DisplayName = "Start Battle With Reason"))
	bool StartBattleWithReason(FName LevelId, FText& OutFailureReason);

	UFUNCTION(BlueprintCallable, Category = "Toy Tower Defense|Battle")
	void EndBattle(ETTDBattleState FinalState);

	UFUNCTION(BlueprintCallable, Category = "Toy Tower Defense|Battle")
	bool TryBuildOnSlot(FName BuildingId, ATTDBuildSlotActor* Slot, FText& OutFailureReason);

	UFUNCTION(BlueprintCallable, Category = "Toy Tower Defense|Battle")
	bool BuyToyBox(FName ToyBoxId, FText& OutFailureReason);

	UFUNCTION(BlueprintCallable, Category = "Toy Tower Defense|Battle")
	bool OpenToyBox(FName ToyBoxId, TArray<FTTDNameStack>& OutRewards, FText& OutFailureReason);

	UFUNCTION(BlueprintPure, Category = "Toy Tower Defense|Battle")
	ETTDBattleState GetBattleState() const { return BattleState; }

	UFUNCTION(BlueprintPure, Category = "Toy Tower Defense|Battle")
	bool IsBattleRunning() const { return BattleState == ETTDBattleState::Running; }

	UFUNCTION(BlueprintPure, Category = "Toy Tower Defense|Battle")
	float GetProgress() const;

	UFUNCTION(BlueprintPure, Category = "Toy Tower Defense|Battle")
	float GetRemainingEnemyWeight() const { return RemainingUnspawnedWeight + AliveEnemyWeight; }

	UFUNCTION(BlueprintPure, Category = "Toy Tower Defense|Battle")
	float GetTotalEnemyWeight() const { return TotalEnemyWeight; }

	UFUNCTION(BlueprintPure, Category = "Toy Tower Defense|Battle")
	int32 GetCurrency() const { return Currency; }

	UFUNCTION(BlueprintPure, Category = "Toy Tower Defense|Battle")
	float GetCastleCurrentHealth() const;

	UFUNCTION(BlueprintPure, Category = "Toy Tower Defense|Battle")
	float GetCastleMaxHealth() const;

	UFUNCTION(BlueprintPure, Category = "Toy Tower Defense|Battle")
	TArray<FTTDBuildingDefinition> GetBuildingDefinitions() const;

	UFUNCTION(BlueprintPure, Category = "Toy Tower Defense|Battle")
	TArray<FTTDToyBoxRewardDefinition> GetToyBoxDefinitions() const;

	UFUNCTION(BlueprintPure, Category = "Toy Tower Defense|Battle")
	TArray<FName> GetDiagramIds() const;

	UFUNCTION(BlueprintPure, Category = "Toy Tower Defense|Battle")
	TArray<FTTDNameStack> GetPartStacks() const;

	UFUNCTION(BlueprintPure, Category = "Toy Tower Defense|Battle")
	TArray<FTTDNameStack> GetToyBoxStacks() const;

	UFUNCTION(BlueprintPure, Category = "Toy Tower Defense|Battle")
	int32 GetActiveProjectileCount() const { return ActiveProjectiles.Num(); }

	UFUNCTION(BlueprintPure, Category = "Toy Tower Defense|Battle")
	int32 GetBattleInstanceId() const { return BattleInstanceId; }

	UFUNCTION(BlueprintCallable, Category = "Toy Tower Defense|Battle")
	void SetRewardRandomSeed(int32 Seed);

	void HandleCastleDestroyed(ATTDBattleCastleActor* DestroyedCastle);
	void HandleBuildingDestroyed(ATTDBattleBuildingActor* DestroyedBuilding);
	void HandleEnemyKilled(ATTDBattleEnemyActor* Enemy);
	void RegisterProjectile(ATTDBattleProjectileActor* Projectile, int32 InBattleInstanceId);
	void UnregisterProjectile(ATTDBattleProjectileActor* Projectile);
	bool IsBattleInstanceCurrent(int32 InBattleInstanceId) const;
	UObject* FindNearestBattleTarget(const FVector& FromLocation) const;
	ATTDBattleEnemyActor* FindNearestEnemy(const FVector& FromLocation, float MaxRange) const;

	const FTTDBuildingDefinition* FindBuildingDefinition(FName BuildingId) const;
	const FTTDToyBoxRewardDefinition* FindToyBoxDefinition(FName ToyBoxId) const;
	FTTDBattleBuildingRuntimeStats BuildRuntimeStatsForSlot(const FTTDBuildingDefinition& BuildingDefinition, const ATTDBuildSlotActor* Slot) const;

private:
	UPROPERTY(Transient)
	TMap<FName, FTTDBattleLevelDefinition> LevelDefinitions;

	UPROPERTY(Transient)
	TMap<FName, FTTDWaveDefinition> WaveDefinitions;

	UPROPERTY(Transient)
	TMap<FName, FTTDEnemyDefinition> EnemyDefinitions;

	UPROPERTY(Transient)
	TMap<FName, FTTDBuildingDefinition> BuildingDefinitions;

	UPROPERTY(Transient)
	TMap<FName, FTTDToyBoxRewardDefinition> ToyBoxRewardDefinitions;

	UPROPERTY(Transient)
	TMap<FName, FTTDBattleHeightEffectDefinition> HeightEffectDefinitions;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ATTDBattleSpawnPointActor>> SpawnPoints;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ATTDBuildSlotActor>> BuildSlots;

	UPROPERTY(Transient)
	TObjectPtr<ATTDBattleCastleActor> CastleActor;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ATTDBattleEnemyActor>> ActiveEnemies;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ATTDBattleBuildingActor>> ActiveBuildings;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ATTDBattleProjectileActor>> ActiveProjectiles;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> BattleTargets;

	UPROPERTY(Transient)
	TArray<FTTDBattlePendingSpawn> PendingSpawns;

	UPROPERTY(Transient)
	TSet<FName> DiagramIds;

	UPROPERTY(Transient)
	TMap<FName, int32> PartCounts;

	UPROPERTY(Transient)
	TMap<FName, int32> ToyBoxCounts;

	FName ActiveLevelId;
	ETTDBattleState BattleState = ETTDBattleState::Inactive;
	FRandomStream RewardRandomStream;
	float BattleElapsedSeconds = 0.0f;
	float TotalEnemyWeight = 0.0f;
	float RemainingUnspawnedWeight = 0.0f;
	float AliveEnemyWeight = 0.0f;
	float FrozenProgress = 0.0f;
	int32 Currency = 0;
	int32 BattleInstanceId = 0;
	bool bBroadcastedInitialState = false;
	bool bUseFrozenProgress = false;

	void LoadDefinitionsFromSettings();
	void GatherPlacedActors();
	void CleanupBattle();
	bool ValidateBattleStart(FName LevelId, FText& OutFailureReason) const;
	bool ValidateLevelDefinition(FName LevelId, const FTTDBattleLevelDefinition& LevelDefinition, FText& OutFailureReason) const;
	bool ValidateEnemyDefinition(FName EnemyId, FText& OutFailureReason) const;
	bool ValidateProjectileBuildingDefinitions(FText& OutFailureReason) const;
	bool HasSpawnPointForGroup(FName SpawnGroup) const;
	bool IsPoolableActorClass(const UClass* ActorClass, const UClass* RequiredBaseClass) const;
	UClass* ResolveEnemyClass(const FTTDEnemyDefinition& EnemyDefinition) const;
	void EnterConfigurationError(const FText& FailureReason);
	void ReleaseActiveEnemies();
	void ReleaseActiveBuildings();
	void ReleaseActiveProjectiles();
	void ReleaseBattleActor(AActor* Actor) const;
	bool SpawnCastleForLevel(const FTTDBattleLevelDefinition& LevelDefinition);
	void BuildSpawnQueueForLevel(const FTTDBattleLevelDefinition& LevelDefinition);
	void SpawnDueEnemies();
	bool SpawnEnemy(const FTTDBattlePendingSpawn& PendingSpawn, FText& OutFailureReason);
	ATTDBattleSpawnPointActor* PickSpawnPoint(FName SpawnGroup) const;
	void CheckVictoryCondition();
	void RegisterBattleTarget(UObject* Target);
	void UnregisterBattleTarget(UObject* Target);
	bool HasPartCosts(const TArray<FTTDNameStack>& PartCosts) const;
	void ApplyPartCosts(const TArray<FTTDNameStack>& PartCosts);
	void AddPartCount(FName PartId, int32 Count);
	void AddToyBoxCount(FName ToyBoxId, int32 Count);
	void BroadcastStateChanged() const;
	void BroadcastProgressChanged() const;
	void BroadcastCurrencyChanged() const;
	void BroadcastInventoryChanged() const;
	void BroadcastBuildingPlaced(FName BuildingId, FName SlotId) const;
	void BroadcastBattleEnded(ETTDBattleState FinalState) const;
};
