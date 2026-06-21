#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "StructUtils/InstancedStruct.h"
#include "WorldConditionBase.h"
#include "WorldConditionSchema.h"
#include "TTDWorldConditionSandbox.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogTTDWorldConditionSandbox, Log, All);

USTRUCT(BlueprintType)
struct TOYTOWERDEFENSE_API FTTDConditionDataBase
{
	GENERATED_BODY()
};

USTRUCT(BlueprintType)
struct TOYTOWERDEFENSE_API FTTDConditionData_HasCurrency : public FTTDConditionDataBase
{
	GENERATED_BODY()

	FTTDConditionData_HasCurrency() = default;

	explicit FTTDConditionData_HasCurrency(const int32 InMinCurrency)
		: MinCurrency(InMinCurrency)
	{
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Toy Tower Defense|WorldCondition Sandbox", meta = (ClampMin = "0"))
	int32 MinCurrency = 0;
};

USTRUCT(BlueprintType)
struct TOYTOWERDEFENSE_API FTTDConditionData_HasDiagram : public FTTDConditionDataBase
{
	GENERATED_BODY()

	FTTDConditionData_HasDiagram() = default;

	explicit FTTDConditionData_HasDiagram(const FName InDiagramId)
		: DiagramId(InDiagramId)
	{
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Toy Tower Defense|WorldCondition Sandbox")
	FName DiagramId;
};

USTRUCT(BlueprintType)
struct TOYTOWERDEFENSE_API FTTDWorldConditionDataRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Toy Tower Defense|WorldCondition Sandbox")
	FName ConditionDataId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Toy Tower Defense|WorldCondition Sandbox", meta = (BaseStruct = "/Script/ToyTowerDefense.TTDConditionDataBase"))
	FInstancedStruct Payload;
};

USTRUCT(BlueprintType)
struct TOYTOWERDEFENSE_API FTTDWorldConditionSandboxContext
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Toy Tower Defense|WorldCondition Sandbox")
	int32 Currency = 0;

	UPROPERTY(EditAnywhere, Category = "Toy Tower Defense|WorldCondition Sandbox")
	TSet<FName> DiagramIds;
};

UCLASS()
class TOYTOWERDEFENSE_API UTTDWorldConditionSandboxSchema : public UWorldConditionSchema
{
	GENERATED_BODY()

public:
	explicit UTTDWorldConditionSandboxSchema(const FObjectInitializer& ObjectInitializer);

	FWorldConditionContextDataRef GetSandboxContextRef() const { return SandboxContextRef; }

protected:
	virtual bool IsStructAllowed(const UScriptStruct* InScriptStruct) const override;

private:
	FWorldConditionContextDataRef SandboxContextRef;
};

USTRUCT()
struct TOYTOWERDEFENSE_API FTTDWorldCondition_HasCurrencyFromRow : public FWorldConditionBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Toy Tower Defense|WorldCondition Sandbox", meta = (RowType = "/Script/ToyTowerDefense.TTDWorldConditionDataRow"))
	FDataTableRowHandle DataRow;

	virtual bool Initialize(const UWorldConditionSchema& Schema) override;
	virtual FWorldConditionResult IsTrue(const FWorldConditionContext& Context) const override;

#if WITH_EDITOR
	virtual FText GetDescription() const override;
#endif

private:
	FWorldConditionContextDataRef SandboxContextRef;
};

USTRUCT()
struct TOYTOWERDEFENSE_API FTTDWorldCondition_HasDiagramFromRow : public FWorldConditionBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Toy Tower Defense|WorldCondition Sandbox", meta = (RowType = "/Script/ToyTowerDefense.TTDWorldConditionDataRow"))
	FDataTableRowHandle DataRow;

	virtual bool Initialize(const UWorldConditionSchema& Schema) override;
	virtual FWorldConditionResult IsTrue(const FWorldConditionContext& Context) const override;

#if WITH_EDITOR
	virtual FText GetDescription() const override;
#endif

private:
	FWorldConditionContextDataRef SandboxContextRef;
};
