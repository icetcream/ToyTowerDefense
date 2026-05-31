#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "TTDBattleTypes.generated.h"

UENUM(BlueprintType)
enum class ETTDBattleState : uint8
{
	Inactive UMETA(DisplayName = "Inactive"),
	Running UMETA(DisplayName = "Running"),
	Victory UMETA(DisplayName = "Victory"),
	Defeat UMETA(DisplayName = "Defeat"),
	ConfigurationError UMETA(DisplayName = "Configuration Error")
};

UENUM(BlueprintType)
enum class ETTDBuildingAttackMode : uint8
{
	InstantDamage UMETA(DisplayName = "Instant Damage"),
	Projectile UMETA(DisplayName = "Projectile")
};

UENUM(BlueprintType)
enum class ETTDBattleAttribute : uint8
{
	AttackDamage UMETA(DisplayName = "Attack Damage"),
	AttackRange UMETA(DisplayName = "Attack Range"),
	AttackInterval UMETA(DisplayName = "Attack Interval"),
	MaxHealth UMETA(DisplayName = "Max Health"),
	ProjectileSpeed UMETA(DisplayName = "Projectile Speed")
};

UENUM(BlueprintType)
enum class ETTDAttributeModifierOp : uint8
{
	Add UMETA(DisplayName = "Add"),
	Multiply UMETA(DisplayName = "Multiply"),
	Override UMETA(DisplayName = "Override")
};

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Toy Tower Defense|Battle")
	FName Id;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Toy Tower Defense|Battle", meta = (ClampMin = "0"))
	int32 Count = 0;
};

USTRUCT(BlueprintType)
struct FTTDBattleAttributeModifier
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle")
	ETTDBattleAttribute Attribute = ETTDBattleAttribute::AttackDamage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle")
	ETTDAttributeModifierOp Operation = ETTDAttributeModifierOp::Add;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle")
	float Value = 0.0f;
};

USTRUCT(BlueprintType)
struct FTTDBattleHeightEffectDefinition : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle")
	FName HeightEffectId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle")
	TArray<FTTDBattleAttributeModifier> Modifiers;
};

USTRUCT(BlueprintType)
struct FTTDBattleBuildingRuntimeStats
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Toy Tower Defense|Battle")
	float MaxHealth = 100.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Toy Tower Defense|Battle")
	float AttackDamage = 10.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Toy Tower Defense|Battle")
	float AttackRange = 600.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Toy Tower Defense|Battle")
	float AttackInterval = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Toy Tower Defense|Battle")
	float ProjectileSpeed = 900.0f;
};

USTRUCT(BlueprintType)
struct FTTDEnemyDefinition : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle")
	FName EnemyId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle")
	TSoftClassPtr<AActor> EnemyClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle", meta = (ClampMin = "1.0"))
	float MaxHealth = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle", meta = (ClampMin = "0.0"))
	float MoveSpeed = 220.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle", meta = (ClampMin = "0.0"))
	float AttackDamage = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle", meta = (ClampMin = "0.0"))
	float AttackRange = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle", meta = (ClampMin = "0.01"))
	float AttackInterval = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle", meta = (ClampMin = "0.0"))
	float ProgressWeight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle", meta = (ClampMin = "0"))
	int32 CurrencyDrop = 1;
};

USTRUCT(BlueprintType)
struct FTTDWaveEnemyEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle")
	FName EnemyId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle", meta = (ClampMin = "0"))
	int32 Count = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle", meta = (ClampMin = "0.01"))
	float SpawnInterval = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle")
	FName SpawnGroup;
};

USTRUCT(BlueprintType)
struct FTTDWaveDefinition : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle")
	FName WaveId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle", meta = (ClampMin = "0.0"))
	float DelayBeforeWave = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle")
	TArray<FTTDWaveEnemyEntry> EnemyEntries;
};

USTRUCT(BlueprintType)
struct FTTDBuildingDefinition : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle")
	FName BuildingId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle")
	FName RequiredDiagramId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle")
	TSoftClassPtr<AActor> BuildingClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle")
	TArray<FTTDNameStack> PartCosts;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle")
	ETTDBuildingAttackMode AttackMode = ETTDBuildingAttackMode::InstantDamage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle")
	FTTDBattleBuildingRuntimeStats BaseStats;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle", meta = (EditCondition = "AttackMode == ETTDBuildingAttackMode::Projectile"))
	TSoftClassPtr<AActor> ProjectileClass;
};

USTRUCT(BlueprintType)
struct FTTDToyBoxRewardEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle")
	FName PartId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle", meta = (ClampMin = "0.0"))
	float Weight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle", meta = (ClampMin = "1"))
	int32 MinQuantity = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle", meta = (ClampMin = "1"))
	int32 MaxQuantity = 1;
};

USTRUCT(BlueprintType)
struct FTTDToyBoxRewardDefinition : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle")
	FName ToyBoxId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle", meta = (ClampMin = "0"))
	int32 PurchaseCost = 5;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle", meta = (ClampMin = "1"))
	int32 RollCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle")
	TArray<FTTDToyBoxRewardEntry> RewardEntries;
};

USTRUCT(BlueprintType)
struct FTTDBattleLevelDefinition : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle")
	FName LevelId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle")
	TSoftClassPtr<AActor> CastleClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle")
	FTransform CastleTransform = FTransform::Identity;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle", meta = (ClampMin = "1.0"))
	float CastleMaxHealth = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle")
	TArray<FName> WaveIds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle")
	TArray<FName> StartingDiagramIds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle")
	TArray<FTTDNameStack> StartingToyBoxes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle")
	TArray<FTTDNameStack> StartingParts;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle", meta = (ClampMin = "0"))
	int32 StartingCurrency = 0;
};
