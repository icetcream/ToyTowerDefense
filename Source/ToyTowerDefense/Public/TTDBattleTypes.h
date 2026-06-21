#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "TTDTypes.h"
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
enum class ETTDBattlePhase : uint8
{
	None UMETA(DisplayName = "None"),
	Preparing UMETA(DisplayName = "Preparing"),
	Buffer UMETA(DisplayName = "Buffer"),
	WaveActive UMETA(DisplayName = "Wave Active"),
	VictoryDelay UMETA(DisplayName = "Victory Delay"),
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
	ProjectileSpeed UMETA(DisplayName = "Projectile Speed"),
	MoveSpeed UMETA(DisplayName = "Move Speed")
};

UENUM(BlueprintType)
enum class ETTDAttributeModifierOp : uint8
{
	Add UMETA(DisplayName = "Add"),
	Multiply UMETA(DisplayName = "Multiply"),
	Override UMETA(DisplayName = "Override")
};

USTRUCT(BlueprintType)
struct FTTDBattleLoadout
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Toy Tower Defense|Battle")
	TArray<FName> SelectedDiagramIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Toy Tower Defense|Battle")
	TArray<FTTDNameStack> SelectedToyBoxes;
};

USTRUCT(BlueprintType)
struct FTTDBattleBackgroundDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle")
	bool bSpawnBackground = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle")
	TSoftClassPtr<AActor> BackgroundClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle")
	FTransform Transform = FTransform::Identity;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle", meta = (ClampMin = "100.0"))
	FVector2D ArenaHalfExtent = FVector2D(6500.0f, 6500.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle", meta = (ClampMin = "1.0"))
	float GroundThickness = 24.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle", meta = (ClampMin = "0.0"))
	float WallHeight = 900.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle", meta = (ClampMin = "0.0"))
	float WallThickness = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle")
	FLinearColor GroundColor = FLinearColor(0.38f, 0.58f, 0.42f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle")
	FLinearColor WallColor = FLinearColor(0.56f, 0.76f, 0.86f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle")
	FLinearColor LightColor = FLinearColor(1.0f, 0.93f, 0.82f, 1.0f);
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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle", meta = (ClampMin = "1.0"))
	float MaxHealth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle", meta = (ClampMin = "0.0"))
	float AttackDamage = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle", meta = (ClampMin = "0.0"))
	float AttackRange = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle", meta = (ClampMin = "0.01"))
	float AttackInterval = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle", meta = (ClampMin = "1.0"))
	float ProjectileSpeed = 900.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle", meta = (ClampMin = "0.0"))
	float MoveSpeed = 0.0f;
};

USTRUCT(BlueprintType)
struct FTTDBattleBuildingUpgradeOption
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle")
	FName PartId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle", meta = (ClampMin = "1"))
	int32 PartCost = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle")
	TArray<FTTDBattleAttributeModifier> Modifiers;
};

USTRUCT(BlueprintType)
struct FTTDEnemyDefinition : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle")
	FName EnemyId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle")
	FText DisplayName;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CurrencyDropChance = 1.0f;
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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle", meta = (ClampMin = "0.0"))
	float WaveDurationSeconds = 0.0f;

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
	FText DisplayName;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle")
	TArray<FTTDBattleBuildingUpgradeOption> UpgradeOptions;
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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle")
	FText DisplayName;

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
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle")
	FTTDBattleBackgroundDefinition Background;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle", meta = (ClampMin = "0"))
	int32 StartingBuildingCapacity = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle", meta = (ClampMin = "0"))
	int32 BuildingCapacityPurchaseCost = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle", meta = (ClampMin = "1"))
	int32 StartingBuildingLevelCap = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle", meta = (ClampMin = "0"))
	int32 BuildingLevelCapPurchaseCost = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle")
	bool bAllowArbitraryBuildPlacement = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle", meta = (ClampMin = "0.0"))
	float ArbitraryBuildMinSpacing = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle", meta = (ClampMin = "0"))
	int32 MaxSelectedDiagrams = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle", meta = (ClampMin = "0"))
	int32 MaxSelectedToyBoxes = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle", meta = (ClampMin = "0.0"))
	float PreparationDurationSeconds = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Battle", meta = (ClampMin = "0.0"))
	float VictoryReturnDelaySeconds = 5.0f;
};
