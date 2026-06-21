#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "TTDTypes.h"
#include "TTDSaveGame.generated.h"

UCLASS()
class TOYTOWERDEFENSE_API UTTDSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	static constexpr int32 CurrentSaveVersion = 2;

	UPROPERTY()
	int32 SaveVersion = 0;

	UPROPERTY()
	TArray<FName> UnlockedDiagramIds;

	UPROPERTY()
	TArray<FName> UnlockedToyBoxIds;

	UPROPERTY()
	TArray<FName> UnlockedPartIds;

	UPROPERTY()
	TArray<FTTDNameStack> PartInventory;

	UPROPERTY()
	TArray<FTTDCraftQueueItem> CraftingQueue;

	UPROPERTY()
	int64 LastSavedUnixSeconds = 0;
};
