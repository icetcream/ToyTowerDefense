#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TTDPooledObjectInterface.generated.h"

UINTERFACE(BlueprintType)
class TOYTOWERDEFENSE_API UTTDPooledObjectInterface : public UInterface
{
	GENERATED_BODY()
};

class TOYTOWERDEFENSE_API ITTDPooledObjectInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Toy Tower Defense|Object Pool")
	void OnPoolCreated(UObject* PoolOwner);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Toy Tower Defense|Object Pool")
	void OnAcquireFromPool(const FTransform& SpawnTransform, UObject* RequestContext);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Toy Tower Defense|Object Pool")
	void OnReleaseToPool();
};
