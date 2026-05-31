#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TTDBattleSpawnPointActor.generated.h"

class UMaterialInterface;

UCLASS(Blueprintable)
class TOYTOWERDEFENSE_API ATTDBattleSpawnPointActor : public AActor
{
	GENERATED_BODY()

public:
	ATTDBattleSpawnPointActor();

	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintPure, Category = "Toy Tower Defense|Battle")
	FName GetSpawnGroup() const { return SpawnGroup; }

	UFUNCTION(BlueprintCallable, Category = "Toy Tower Defense|Battle")
	void ConfigureSpawnPoint(FName InSpawnGroup);

private:
	UPROPERTY(VisibleAnywhere, Category = "Toy Tower Defense|Battle")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Toy Tower Defense|Battle")
	TObjectPtr<class UStaticMeshComponent> VisualMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Toy Tower Defense|Battle", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMaterialInterface> VisualMaterial;

	UPROPERTY(EditAnywhere, Category = "Toy Tower Defense|Battle")
	FName SpawnGroup;

	void ApplyVisualMaterial();
};
