#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TTDBuildSlotActor.generated.h"

class UBoxComponent;
class UMaterialInterface;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class TOYTOWERDEFENSE_API ATTDBuildSlotActor : public AActor
{
	GENERATED_BODY()

public:
	ATTDBuildSlotActor();

	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintPure, Category = "Toy Tower Defense|Battle")
	FName GetSlotId() const { return SlotId; }

	UFUNCTION(BlueprintPure, Category = "Toy Tower Defense|Battle")
	FIntPoint GetGridCoordinate() const { return GridCoordinate; }

	UFUNCTION(BlueprintPure, Category = "Toy Tower Defense|Battle")
	int32 GetHeightLevel() const { return HeightLevel; }

	UFUNCTION(BlueprintPure, Category = "Toy Tower Defense|Battle")
	FName GetHeightEffectId() const { return HeightEffectId; }

	UFUNCTION(BlueprintPure, Category = "Toy Tower Defense|Battle")
	bool IsOccupied() const { return OccupyingBuilding.IsValid(); }

	UFUNCTION(BlueprintPure, Category = "Toy Tower Defense|Battle")
	AActor* GetOccupyingBuilding() const { return OccupyingBuilding.Get(); }

	UFUNCTION(BlueprintCallable, Category = "Toy Tower Defense|Battle")
	void ConfigureSlot(FName InSlotId, FIntPoint InGridCoordinate, int32 InHeightLevel, FName InHeightEffectId);

	void SetOccupyingBuilding(AActor* Building);
	void ClearOccupyingBuilding(AActor* Building);

private:
	UPROPERTY(VisibleAnywhere, Category = "Toy Tower Defense|Battle")
	TObjectPtr<UBoxComponent> SlotBounds;

	UPROPERTY(VisibleAnywhere, Category = "Toy Tower Defense|Battle")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Toy Tower Defense|Battle", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMaterialInterface> VisualMaterial;

	UPROPERTY(EditAnywhere, Category = "Toy Tower Defense|Battle")
	FName SlotId;

	UPROPERTY(EditAnywhere, Category = "Toy Tower Defense|Battle")
	FIntPoint GridCoordinate = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, Category = "Toy Tower Defense|Battle")
	int32 HeightLevel = 0;

	UPROPERTY(EditAnywhere, Category = "Toy Tower Defense|Battle")
	FName HeightEffectId;

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> OccupyingBuilding;

	void ApplyVisualMaterial();
};
