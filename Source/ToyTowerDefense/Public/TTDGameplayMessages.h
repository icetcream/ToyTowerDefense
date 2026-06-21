#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "NativeGameplayTags.h"
#include "TTDBattleTypes.h"
#include "TTDTypes.h"
#include "TTDGameplayMessages.generated.h"

TOYTOWERDEFENSE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_TTD_Message_Research_Queue_Changed);
TOYTOWERDEFENSE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_TTD_Message_Research_Queue_Enqueued);
TOYTOWERDEFENSE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_TTD_Message_Research_Queue_Claimed);
TOYTOWERDEFENSE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_TTD_Message_Research_Collection_Changed);
TOYTOWERDEFENSE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_TTD_Message_Research_Inventory_Changed);
TOYTOWERDEFENSE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_TTD_Message_Battle_State_Changed);
TOYTOWERDEFENSE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_TTD_Message_Battle_Phase_Changed);
TOYTOWERDEFENSE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_TTD_Message_Battle_Progress_Changed);
TOYTOWERDEFENSE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_TTD_Message_Battle_Currency_Changed);
TOYTOWERDEFENSE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_TTD_Message_Battle_Inventory_Changed);
TOYTOWERDEFENSE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_TTD_Message_Battle_CastleHealth_Changed);
TOYTOWERDEFENSE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_TTD_Message_Battle_Building_Placed);
TOYTOWERDEFENSE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_TTD_Message_Battle_Ended);

USTRUCT(BlueprintType)
struct FTTDResearchQueueChangedMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Toy Tower Defense|Messaging")
	TArray<FTTDCraftQueueItem> QueueItems;

	UPROPERTY(BlueprintReadOnly, Category = "Toy Tower Defense|Messaging")
	int32 MaxSlots = 0;
};

USTRUCT(BlueprintType)
struct FTTDToyBoxQueuedMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Toy Tower Defense|Messaging")
	FTTDCraftQueueItem QueueItem;
};

USTRUCT(BlueprintType)
struct FTTDToyBoxClaimedMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Toy Tower Defense|Messaging")
	FTTDCraftQueueItem QueueItem;

	UPROPERTY(BlueprintReadOnly, Category = "Toy Tower Defense|Messaging")
	TArray<FTTDPartDefinition> NewlyUnlockedParts;

	UPROPERTY(BlueprintReadOnly, Category = "Toy Tower Defense|Messaging")
	TArray<FTTDNameStack> GrantedParts;

	UPROPERTY(BlueprintReadOnly, Category = "Toy Tower Defense|Messaging")
	TArray<FTTDNameStack> GrantedToyBoxes;

	UPROPERTY(BlueprintReadOnly, Category = "Toy Tower Defense|Messaging")
	TArray<FTTDNameStack> PartInventory;

	UPROPERTY(BlueprintReadOnly, Category = "Toy Tower Defense|Messaging")
	TArray<FTTDNameStack> ToyBoxInventory;
};

USTRUCT(BlueprintType)
struct FTTDResearchCollectionChangedMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Toy Tower Defense|Messaging")
	TArray<FName> UnlockedDiagramIds;

	UPROPERTY(BlueprintReadOnly, Category = "Toy Tower Defense|Messaging")
	TArray<FName> UnlockedToyBoxIds;

	UPROPERTY(BlueprintReadOnly, Category = "Toy Tower Defense|Messaging")
	TArray<FName> UnlockedPartIds;
};

USTRUCT(BlueprintType)
struct FTTDResearchInventoryChangedMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Toy Tower Defense|Messaging")
	TArray<FTTDNameStack> PartInventory;

	UPROPERTY(BlueprintReadOnly, Category = "Toy Tower Defense|Messaging")
	TArray<FTTDNameStack> ToyBoxInventory;
};

USTRUCT(BlueprintType)
struct FTTDBattleStateChangedMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Toy Tower Defense|Messaging")
	FName LevelId;

	UPROPERTY(BlueprintReadOnly, Category = "Toy Tower Defense|Messaging")
	ETTDBattleState State = ETTDBattleState::Inactive;
};

USTRUCT(BlueprintType)
struct FTTDBattlePhaseChangedMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Toy Tower Defense|Messaging")
	FName LevelId;

	UPROPERTY(BlueprintReadOnly, Category = "Toy Tower Defense|Messaging")
	ETTDBattlePhase Phase = ETTDBattlePhase::None;

	UPROPERTY(BlueprintReadOnly, Category = "Toy Tower Defense|Messaging")
	int32 CurrentWaveNumber = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Toy Tower Defense|Messaging")
	int32 TotalWaveCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Toy Tower Defense|Messaging")
	float RemainingSeconds = 0.0f;
};

USTRUCT(BlueprintType)
struct FTTDBattleProgressChangedMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Toy Tower Defense|Messaging")
	float Progress = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Toy Tower Defense|Messaging")
	float RemainingWeight = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Toy Tower Defense|Messaging")
	float TotalWeight = 0.0f;
};

USTRUCT(BlueprintType)
struct FTTDBattleCurrencyChangedMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Toy Tower Defense|Messaging")
	int32 Currency = 0;
};

USTRUCT(BlueprintType)
struct FTTDBattleInventoryChangedMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Toy Tower Defense|Messaging")
	TArray<FName> DiagramIds;

	UPROPERTY(BlueprintReadOnly, Category = "Toy Tower Defense|Messaging")
	TArray<FTTDNameStack> ToyBoxes;

	UPROPERTY(BlueprintReadOnly, Category = "Toy Tower Defense|Messaging")
	TArray<FTTDNameStack> Parts;
};

USTRUCT(BlueprintType)
struct FTTDBattleCastleHealthChangedMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Toy Tower Defense|Messaging")
	float CurrentHealth = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Toy Tower Defense|Messaging")
	float MaxHealth = 0.0f;
};

USTRUCT(BlueprintType)
struct FTTDBattleBuildingPlacedMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Toy Tower Defense|Messaging")
	FName BuildingId;

	UPROPERTY(BlueprintReadOnly, Category = "Toy Tower Defense|Messaging")
	FName SlotId;
};

USTRUCT(BlueprintType)
struct FTTDBattleEndedMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Toy Tower Defense|Messaging")
	FName LevelId;

	UPROPERTY(BlueprintReadOnly, Category = "Toy Tower Defense|Messaging")
	ETTDBattleState FinalState = ETTDBattleState::Inactive;
};
