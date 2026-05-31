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

	void InitializeBuilding(FName InBuildingId, const FTTDBuildingDefinition& Definition, const FTTDBattleBuildingRuntimeStats& InRuntimeStats, ATTDBuildSlotActor* InOwningSlot);

	UFUNCTION(BlueprintPure, Category = "Toy Tower Defense|Battle")
	FName GetBuildingId() const { return BuildingId; }

	UFUNCTION(BlueprintPure, Category = "Toy Tower Defense|Battle")
	FTTDBattleBuildingRuntimeStats GetRuntimeStats() const { return RuntimeStats; }

	UFUNCTION(BlueprintPure, Category = "Toy Tower Defense|Battle")
	float GetCurrentHealth() const { return CurrentHealth; }

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
	ETTDBuildingAttackMode AttackMode = ETTDBuildingAttackMode::InstantDamage;
	FTTDBattleBuildingRuntimeStats RuntimeStats;
	TSoftClassPtr<AActor> ProjectileClass;
	float CurrentHealth = 100.0f;
	float AttackAccumulator = 0.0f;

	void TryAttack(float DeltaSeconds);
	void FireInstantDamage(ATTDBattleEnemyActor* Target);
	void FireProjectile(ATTDBattleEnemyActor* Target);
	void RefreshVisualState();
	void ApplyVisualMaterial();
};
