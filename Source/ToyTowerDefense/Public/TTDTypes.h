#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "TTDTypes.generated.h"

USTRUCT(BlueprintType)
struct FTTDNameStack
{
	GENERATED_BODY()

	FTTDNameStack() = default;

	FTTDNameStack(const FName InId, const int32 InCount)
		: Id(InId)
		, Count(InCount)
	{
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Toy Tower Defense")
	FName Id;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Toy Tower Defense", meta = (ClampMin = "0"))
	int32 Count = 0;
};

UENUM(BlueprintType)
enum class ETTDCollectionCategory : uint8
{
	Diagram UMETA(DisplayName = "Diagram"),
	ToyBox UMETA(DisplayName = "Toy Box")
};

USTRUCT(BlueprintType)
struct FTTDPartDefinition : public FTableRowBase
{
	GENERATED_BODY()

	FTTDPartDefinition() = default;

	FTTDPartDefinition(const FName InPartId, const FText& InDisplayName, const FText& InDescription)
		: PartId(InPartId)
		, DisplayName(InDisplayName)
		, Description(InDescription)
	{
	}

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense")
	FName PartId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense")
	FText Description;
};

USTRUCT(BlueprintType)
struct FTTDDiagramDefinition : public FTableRowBase
{
	GENERATED_BODY()

	FTTDDiagramDefinition() = default;

	FTTDDiagramDefinition(const FName InDiagramId, const FText& InDisplayName, const FText& InDescription, TArray<FName> InRequiredPartIds)
		: DiagramId(InDiagramId)
		, DisplayName(InDisplayName)
		, Description(InDescription)
		, RequiredPartIds(MoveTemp(InRequiredPartIds))
	{
	}

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense")
	FName DiagramId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense")
	TArray<FName> RequiredPartIds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense")
	TArray<FTTDNameStack> PartCosts;
};

USTRUCT(BlueprintType)
struct FTTDToyBoxDefinition : public FTableRowBase
{
	GENERATED_BODY()

	FTTDToyBoxDefinition() = default;

	FTTDToyBoxDefinition(const FName InToyBoxId, const FText& InDisplayName, const FText& InDescription, TArray<FName> InPossiblePartIds, const float InCraftDurationSeconds)
		: ToyBoxId(InToyBoxId)
		, DisplayName(InDisplayName)
		, Description(InDescription)
		, PossiblePartIds(MoveTemp(InPossiblePartIds))
		, CraftDurationSeconds(InCraftDurationSeconds)
	{
	}

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense")
	FName ToyBoxId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense")
	TArray<FName> PossiblePartIds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense", meta = (ClampMin = "0.0"))
	float CraftDurationSeconds = 30.0f;
};

USTRUCT(BlueprintType)
struct FTTDCollectionEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Toy Tower Defense")
	ETTDCollectionCategory Category = ETTDCollectionCategory::Diagram;

	UPROPERTY(BlueprintReadOnly, Category = "Toy Tower Defense")
	FName ItemId;

	UPROPERTY(BlueprintReadOnly, Category = "Toy Tower Defense")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Toy Tower Defense")
	FText Description;

	UPROPERTY(BlueprintReadOnly, Category = "Toy Tower Defense")
	TArray<FName> PartIds;

	UPROPERTY(BlueprintReadOnly, Category = "Toy Tower Defense")
	float CraftDurationSeconds = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Toy Tower Defense")
	bool bUnlocked = false;
};

USTRUCT(BlueprintType)
struct FTTDCraftQueueItem
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Toy Tower Defense")
	FGuid QueueId;

	UPROPERTY(BlueprintReadOnly, Category = "Toy Tower Defense")
	FName ToyBoxId;

	UPROPERTY(BlueprintReadOnly, Category = "Toy Tower Defense")
	FText ToyBoxName;

	UPROPERTY(BlueprintReadOnly, Category = "Toy Tower Defense")
	float DurationSeconds = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Toy Tower Defense")
	float RemainingSeconds = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Toy Tower Defense")
	bool bCompleted = false;
};
