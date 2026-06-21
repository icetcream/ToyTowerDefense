#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Engine/DeveloperSettings.h"
#include "GameplayTagContainer.h"
#include "TTDDeveloperSettings.generated.h"

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Toy Tower Defense"))
class TOYTOWERDEFENSE_API UTTDDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UTTDDeveloperSettings();

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Data Tables", meta = (Categories = "TTD.DataTable"))
	TMap<FGameplayTag, TSoftObjectPtr<UDataTable>> GameplayTagDataTables;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Research", meta = (ClampMin = "1"))
	int32 CraftingQueueMaxSlots = 5;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Save")
	FString SaveSlotName = TEXT("ToyTowerDefenseResearch");

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Research|Defaults")
	TArray<FName> DefaultUnlockedDiagramIds;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Research|Defaults")
	TArray<FName> DefaultUnlockedToyBoxIds;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Research|Defaults")
	TArray<FName> DefaultUnlockedPartIds;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Battle")
	FName DefaultBattleLevelId = TEXT("PrototypeBattle");

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Battle|Debug")
	bool bShowBattleTestCheatButton = false;

	UFUNCTION(BlueprintPure, Category = "Toy Tower Defense|Settings")
	UDataTable* GetDataTableForTag(FGameplayTag DataTableTag) const;
};
