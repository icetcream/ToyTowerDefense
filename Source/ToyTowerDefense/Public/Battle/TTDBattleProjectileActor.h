#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TTDPooledObjectInterface.h"
#include "TTDBattleProjectileActor.generated.h"

class ATTDBattleEnemyActor;
class UMaterialInterface;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class TOYTOWERDEFENSE_API ATTDBattleProjectileActor : public AActor, public ITTDPooledObjectInterface
{
	GENERATED_BODY()

public:
	ATTDBattleProjectileActor();

	virtual void Tick(float DeltaSeconds) override;
	virtual void OnConstruction(const FTransform& Transform) override;

	void InitializeProjectile(ATTDBattleEnemyActor* InTarget, float InDamage, float InSpeed, UObject* InDamageSource, int32 InBattleInstanceId);

	UFUNCTION(BlueprintPure, Category = "Toy Tower Defense|Battle")
	int32 GetBattleInstanceId() const { return BattleInstanceId; }

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
	TWeakObjectPtr<ATTDBattleEnemyActor> Target;

	UPROPERTY(Transient)
	TWeakObjectPtr<UObject> DamageSource;

	float Damage = 0.0f;
	float Speed = 900.0f;
	float HitRadius = 35.0f;
	int32 BattleInstanceId = INDEX_NONE;

	void ReleaseSelf();
	void ApplyVisualMaterial();
};
