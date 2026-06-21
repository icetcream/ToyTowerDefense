#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TTDBattleTypes.h"
#include "TTDBattleBackgroundActor.generated.h"

class UDirectionalLightComponent;
class UMaterialInterface;
class USceneComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class TOYTOWERDEFENSE_API ATTDBattleBackgroundActor : public AActor
{
	GENERATED_BODY()

public:
	ATTDBattleBackgroundActor();

	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintCallable, Category = "Toy Tower Defense|Battle")
	void ConfigureBackground(const FTTDBattleBackgroundDefinition& InDefinition);

	UFUNCTION(BlueprintPure, Category = "Toy Tower Defense|Battle")
	const FTTDBattleBackgroundDefinition& GetBackgroundDefinition() const { return Definition; }

private:
	UPROPERTY(VisibleAnywhere, Category = "Toy Tower Defense|Battle")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Toy Tower Defense|Battle")
	TObjectPtr<UStaticMeshComponent> GroundMesh;

	UPROPERTY(VisibleAnywhere, Category = "Toy Tower Defense|Battle")
	TObjectPtr<UStaticMeshComponent> NorthWallMesh;

	UPROPERTY(VisibleAnywhere, Category = "Toy Tower Defense|Battle")
	TObjectPtr<UStaticMeshComponent> SouthWallMesh;

	UPROPERTY(VisibleAnywhere, Category = "Toy Tower Defense|Battle")
	TObjectPtr<UStaticMeshComponent> EastWallMesh;

	UPROPERTY(VisibleAnywhere, Category = "Toy Tower Defense|Battle")
	TObjectPtr<UStaticMeshComponent> WestWallMesh;

	UPROPERTY(VisibleAnywhere, Category = "Toy Tower Defense|Battle")
	TObjectPtr<UDirectionalLightComponent> KeyLight;

	UPROPERTY(EditDefaultsOnly, Category = "Toy Tower Defense|Battle", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMaterialInterface> BaseMaterial;

	UPROPERTY(EditAnywhere, Category = "Toy Tower Defense|Battle", meta = (AllowPrivateAccess = "true"))
	FTTDBattleBackgroundDefinition Definition;

	void ApplyDefinition();
	void ConfigureCubeMesh(UStaticMeshComponent* MeshComponent) const;
	void ApplyMeshColor(UStaticMeshComponent* MeshComponent, const FLinearColor& Color) const;
};
