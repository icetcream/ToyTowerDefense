#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "TTDObjectPoolTypes.generated.h"

USTRUCT(BlueprintType)
struct FTTDObjectPoolDefinition : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Object Pool")
	TSoftClassPtr<UObject> ObjectClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Object Pool", meta = (ClampMin = "0"))
	int32 InitialSize = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Object Pool", meta = (ClampMin = "0"))
	int32 MaxSize = 32;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Object Pool", meta = (ClampMin = "1"))
	int32 ExpandBy = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toy Tower Defense|Object Pool")
	bool bPrewarmOnWorldBeginPlay = true;
};

USTRUCT(BlueprintType)
struct FTTDObjectPoolStats
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Toy Tower Defense|Object Pool")
	TSubclassOf<UObject> ObjectClass;

	UPROPERTY(BlueprintReadOnly, Category = "Toy Tower Defense|Object Pool")
	int32 AvailableCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Toy Tower Defense|Object Pool")
	int32 ActiveCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Toy Tower Defense|Object Pool")
	int32 TotalPooledCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Toy Tower Defense|Object Pool")
	int32 MaxSize = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Toy Tower Defense|Object Pool")
	bool bConfigured = false;
};
