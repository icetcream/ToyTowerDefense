#pragma once

#include "CoreMinimal.h"
#include "Battle/TTDBattleTargetInterface.h"
#include "GameFramework/Actor.h"
#include "TTDBattleCastleActor.generated.h"

class UMaterialInterface;

UCLASS(Blueprintable)
class TOYTOWERDEFENSE_API ATTDBattleCastleActor : public AActor, public ITTDBattleTargetInterface
{
	GENERATED_BODY()

public:
	ATTDBattleCastleActor();

	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintCallable, Category = "Toy Tower Defense|Battle")
	void InitializeCastle(float InMaxHealth);

	UFUNCTION(BlueprintPure, Category = "Toy Tower Defense|Battle")
	float GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintPure, Category = "Toy Tower Defense|Battle")
	float GetMaxHealth() const { return MaxHealth; }

	virtual FVector GetBattleTargetLocation_Implementation() const override;
	virtual bool IsBattleTargetAlive_Implementation() const override;
	virtual void ApplyBattleDamage_Implementation(float Damage, UObject* DamageSource) override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Toy Tower Defense|Battle")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Toy Tower Defense|Battle")
	TObjectPtr<class UStaticMeshComponent> VisualMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Toy Tower Defense|Battle", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMaterialInterface> VisualMaterial;

	UPROPERTY(EditAnywhere, Category = "Toy Tower Defense|Battle", meta = (ClampMin = "1.0"))
	float MaxHealth = 500.0f;

	UPROPERTY(Transient)
	float CurrentHealth = 500.0f;

	void ApplyVisualMaterial();
};
