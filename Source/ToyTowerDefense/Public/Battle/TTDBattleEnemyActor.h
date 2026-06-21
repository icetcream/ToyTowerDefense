#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TTDBattleTypes.h"
#include "TTDPooledObjectInterface.h"
#include "TTDBattleEnemyActor.generated.h"

class UTTDBattleWorldSubsystem;
class UMaterialInterface;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class TOYTOWERDEFENSE_API ATTDBattleEnemyActor : public AActor, public ITTDPooledObjectInterface
{
	GENERATED_BODY()

public:
	ATTDBattleEnemyActor();

	virtual void Tick(float DeltaSeconds) override;
	virtual void OnConstruction(const FTransform& Transform) override;

	void InitializeEnemy(const FTTDEnemyDefinition& Definition);
	void ApplyDamage(float Damage, UObject* DamageSource);

	UFUNCTION(BlueprintPure, Category = "Toy Tower Defense|Battle")
	bool IsAlive() const { return CurrentHealth > 0.0f; }

	UFUNCTION(BlueprintPure, Category = "Toy Tower Defense|Battle")
	float GetProgressWeight() const { return ProgressWeight; }

	UFUNCTION(BlueprintPure, Category = "Toy Tower Defense|Battle")
	int32 GetCurrencyDrop() const { return CurrencyDrop; }

	UFUNCTION(BlueprintPure, Category = "Toy Tower Defense|Battle")
	float GetCurrencyDropChance() const { return CurrencyDropChance; }

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

	FName EnemyId;
	float MaxHealth = 30.0f;
	float CurrentHealth = 30.0f;
	float MoveSpeed = 220.0f;
	float AttackDamage = 5.0f;
	float AttackRange = 100.0f;
	float AttackInterval = 1.0f;
	float AttackAccumulator = 0.0f;
	float ProgressWeight = 1.0f;
	int32 CurrencyDrop = 1;
	float CurrencyDropChance = 1.0f;

	void MoveAndAttack(float DeltaSeconds, UTTDBattleWorldSubsystem& BattleSubsystem);
	void ApplyVisualMaterial();
};
