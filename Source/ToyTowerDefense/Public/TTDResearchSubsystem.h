#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "TTDCraftingQueueModel.h"
#include "TTDResearchSubsystem.generated.h"

class UTTDSaveGame;
class UTTDDeveloperSettings;

UCLASS(BlueprintType)
class TOYTOWERDEFENSE_API UTTDResearchSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override;
	virtual UWorld* GetTickableGameObjectWorld() const override;

	UFUNCTION(BlueprintCallable, Category = "Toy Tower Defense|Research")
	void ResetProgress();

	UFUNCTION(BlueprintPure, Category = "Toy Tower Defense|Research")
	TArray<FTTDCollectionEntry> GetCollectionEntries(ETTDCollectionCategory Category) const;

	UFUNCTION(BlueprintPure, Category = "Toy Tower Defense|Research")
	TArray<FTTDToyBoxDefinition> GetToyBoxes() const;

	UFUNCTION(BlueprintPure, Category = "Toy Tower Defense|Research")
	TArray<FTTDDiagramDefinition> GetDiagrams() const;

	UFUNCTION(BlueprintPure, Category = "Toy Tower Defense|Research")
	TArray<FTTDPartDefinition> GetParts() const;

	UFUNCTION(BlueprintPure, Category = "Toy Tower Defense|Research")
	TArray<FTTDPartDefinition> GetPartsForToyBox(FName ToyBoxId) const;

	UFUNCTION(BlueprintPure, Category = "Toy Tower Defense|Research")
	TArray<FTTDNameStack> GetPartInventory() const;

	UFUNCTION(BlueprintPure, Category = "Toy Tower Defense|Research")
	int32 GetPartCount(FName PartId) const;

	UFUNCTION(BlueprintPure, Category = "Toy Tower Defense|Research")
	TArray<FTTDCraftQueueItem> GetCraftingQueue() const;

	UFUNCTION(BlueprintPure, Category = "Toy Tower Defense|Research")
	int32 GetCraftingQueueMaxSlots() const;

	UFUNCTION(BlueprintCallable, Category = "Toy Tower Defense|Research")
	bool EnqueueToyBox(FName ToyBoxId, FText& OutFailureReason);

	UFUNCTION(BlueprintCallable, Category = "Toy Tower Defense|Research")
	bool ClaimCompletedToyBox(FGuid QueueId, TArray<FTTDPartDefinition>& OutUnlockedParts);

	UFUNCTION(BlueprintCallable, Category = "Toy Tower Defense|Research")
	bool CanResearchDiagram(FName DiagramId, FText& OutFailureReason) const;

	UFUNCTION(BlueprintCallable, Category = "Toy Tower Defense|Research")
	bool ResearchDiagram(FName DiagramId, FText& OutFailureReason);

	UFUNCTION(BlueprintCallable, Category = "Toy Tower Defense|Research")
	void BroadcastCurrentResearchState() const;

private:
	static constexpr int32 UserIndex = 0;

	UPROPERTY(Transient)
	TObjectPtr<UTTDSaveGame> ActiveSave;

	UPROPERTY(Transient)
	TArray<FTTDPartDefinition> Parts;

	UPROPERTY(Transient)
	TArray<FTTDDiagramDefinition> Diagrams;

	UPROPERTY(Transient)
	TArray<FTTDToyBoxDefinition> ToyBoxes;

	FTTDCraftingQueueModel CraftingQueue;
	float AutosaveAccumulatorSeconds = 0.0f;
	bool bProgressDirty = false;

	void SeedDefinitions();
	void LoadDefinitionsFromSettings(const UTTDDeveloperSettings& Settings);
	void LoadOrCreateProgress();
	void ApplyDefaultUnlocks(const UTTDDeveloperSettings& Settings);
	void ApplyOfflineCraftingProgress();
	void SaveProgress();
	void BroadcastQueueChanged() const;
	void BroadcastToyBoxQueued(const FTTDCraftQueueItem& QueueItem) const;
	void BroadcastToyBoxClaimed(const FTTDCraftQueueItem& QueueItem, const TArray<FTTDPartDefinition>& NewlyUnlockedParts, const TArray<FTTDNameStack>& GrantedParts) const;
	void BroadcastCollectionChanged() const;
	void BroadcastInventoryChanged() const;

	const FTTDToyBoxDefinition* FindToyBox(FName ToyBoxId) const;
	const FTTDDiagramDefinition* FindDiagram(FName DiagramId) const;
	const FTTDPartDefinition* FindPart(FName PartId) const;
	FString GetSaveSlotName() const;
	bool IsUnlocked(const TArray<FName>& UnlockedIds, FName Id) const;
	TArray<FTTDNameStack> GetDiagramPartCosts(const FTTDDiagramDefinition& Diagram) const;
	bool HasPartCosts(const TArray<FTTDNameStack>& PartCosts, FText& OutFailureReason) const;
	void ApplyPartCosts(const TArray<FTTDNameStack>& PartCosts);
	void AddPartCount(FName PartId, int32 Count);
	void NormalizePartInventory();
};
