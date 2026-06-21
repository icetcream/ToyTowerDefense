#pragma once

#include "CoreMinimal.h"
#include "Battle/TTDBattleTargetInterface.h"
#include "GameFramework/Actor.h"
#include "TTDBattleTypes.h"
#include "TTDPooledObjectInterface.h"
#include "TTDBattleBuildingActor.generated.h"

class ATTDBattleEnemyActor;
class ATTDBuildSlotActor;
class UMaterialInterface;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class TOYTOWERDEFENSE_API ATTDBattleBuildingActor : public AActor, public ITTDBattleTargetInterface, public ITTDPooledObjectInterface
{
	GENERATED_BODY()

public:
	ATTDBattleBuildingActor();

	virtual void Tick(float DeltaSeconds) override;
	virtual void OnConstruction(const FTransform& Transform) override;

	void InitializeBuilding(
		FName InBuildingId,
		const FTTDBuildingDefinition& Definition,
		const FTTDBattleBuildingRuntimeStats& InRuntimeStats,
		const TArray<FTTDNameStack>& InConstructionPartCosts,
		ATTDBuildSlotActor* InOwningSlot);

	void ApplyRuntimeStats(const FTTDBattleBuildingRuntimeStats& InRuntimeStats);
	void IncrementLevel();
	void AddUpgradePartCost(FName PartId, int32 Count);

	UFUNCTION(BlueprintPure, Category = "Toy Tower Defense|Battle")
	FName GetBuildingId() const { return BuildingId; }

	UFUNCTION(BlueprintPure, Category = "Toy Tower Defense|Battle")
	FText GetDisplayName() const { return DisplayName; }

	UFUNCTION(BlueprintPure, Category = "Toy Tower Defense|Battle")
	int32 GetBuildingLevel() const { return BuildingLevel; }

	UFUNCTION(BlueprintPure, Category = "Toy Tower Defense|Battle")
	FTTDBattleBuildingRuntimeStats GetRuntimeStats() const { return RuntimeStats; }

	UFUNCTION(BlueprintPure, Category = "Toy Tower Defense|Battle")
	float GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintPure, Category = "Toy Tower Defense|Battle")
	TArray<FTTDNameStack> GetConstructionPartCosts() const { return ConstructionPartCosts; }

	UFUNCTION(BlueprintPure, Category = "Toy Tower Defense|Battle")
	TArray<FTTDNameStack> GetUpgradePartCosts() const { return UpgradePartCosts; }

	UFUNCTION(BlueprintPure, Category = "Toy Tower Defense|Battle")
	ATTDBuildSlotActor* GetOwningSlot() const { return OwningSlot.Get(); }

	virtual FVector GetBattleTargetLocation_Implementation() const override;
	virtual bool IsBattleTargetAlive_Implementation() const override;
	virtual void ApplyBattleDamage_Implementation(float Damage, UObject* DamageSource) override;

	virtual void OnPoolCreated_Implementation(UObject* PoolOwner) override;
	virtual void OnAcquireFromPool_Implementation(const FTransform& SpawnTransform, UObject* RequestContext) override;
	virtual void OnReleaseToPool_Implementation() override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Toy Tower Defense|Battle")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Toy Tower Defense|Battle")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Toy Tower Defense|Battle", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMaterialInterface> VisualMaterial;

	UPROPERTY(Transient)
	TWeakObjectPtr<ATTDBuildSlotActor> OwningSlot;

	FName BuildingId;
	FText DisplayName;
	ETTDBuildingAttackMode AttackMode = ETTDBuildingAttackMode::InstantDamage;
	FTTDBattleBuildingRuntimeStats RuntimeStats;
	TSoftClassPtr<AActor> ProjectileClass;
	TArray<FTTDNameStack> ConstructionPartCosts;
	TArray<FTTDNameStack> UpgradePartCosts;
	float CurrentHealth = 100.0f;
	float AttackAccumulator = 0.0f;
	int32 BuildingLevel = 1;

	void TryAttack(float DeltaSeconds);
	void FireInstantDamage(ATTDBattleEnemyActor* Target);
	void FireProjectile(ATTDBattleEnemyActor* Target);
	void RefreshVisualState();
	void ApplyVisualMaterial();
};
