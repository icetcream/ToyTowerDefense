#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TTDObjectPoolTypes.h"
#include "TTDObjectPoolWorldSubsystem.generated.h"

USTRUCT()
struct FTTDObjectPoolRuntimeState
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TObjectPtr<UClass> ObjectClass;

	UPROPERTY(Transient)
	FTTDObjectPoolDefinition Definition;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> AvailableObjects;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> ActiveObjects;
};

UCLASS(BlueprintType)
class TOYTOWERDEFENSE_API UTTDObjectPoolWorldSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	UFUNCTION(BlueprintCallable, Category = "Toy Tower Defense|Object Pool", meta = (DeterminesOutputType = "ObjectClass"))
	UObject* AcquireObject(TSubclassOf<UObject> ObjectClass, const FTransform& SpawnTransform, UObject* RequestContext, bool& bFromPool);

	UFUNCTION(BlueprintCallable, Category = "Toy Tower Defense|Object Pool")
	AActor* AcquireActor(TSubclassOf<AActor> ActorClass, const FTransform& SpawnTransform, UObject* RequestContext, bool& bFromPool);

	UFUNCTION(BlueprintCallable, Category = "Toy Tower Defense|Object Pool")
	bool ReleaseObject(UObject* Object);

	UFUNCTION(BlueprintCallable, Category = "Toy Tower Defense|Object Pool")
	int32 PrewarmPool(TSubclassOf<UObject> ObjectClass, int32 Count);

	UFUNCTION(BlueprintPure, Category = "Toy Tower Defense|Object Pool")
	FTTDObjectPoolStats GetPoolStats(TSubclassOf<UObject> ObjectClass) const;

	template<typename ObjectType>
	ObjectType* Acquire(const FTransform& SpawnTransform = FTransform::Identity, UObject* RequestContext = nullptr, bool* bOutFromPool = nullptr)
	{
		bool bFromPool = false;
		UObject* Object = AcquireObject(ObjectType::StaticClass(), SpawnTransform, RequestContext, bFromPool);
		if (bOutFromPool)
		{
			*bOutFromPool = bFromPool;
		}
		return Cast<ObjectType>(Object);
	}

	template<typename ObjectType>
	bool Release(ObjectType* Object)
	{
		return ReleaseObject(Object);
	}

private:
	UPROPERTY(Transient)
	TMap<TObjectPtr<UClass>, FTTDObjectPoolRuntimeState> PoolStates;

	void LoadPoolDefinitionsFromSettings();
	bool IsPoolableClass(const UClass* ObjectClass) const;
	FTTDObjectPoolRuntimeState* FindPoolState(const UClass* ObjectClass);
	const FTTDObjectPoolRuntimeState* FindPoolState(const UClass* ObjectClass) const;
	UObject* AcquireFromPool(FTTDObjectPoolRuntimeState& PoolState, const FTransform& SpawnTransform, UObject* RequestContext, bool& bFromPool);
	UObject* CreatePooledInstance(UClass* ObjectClass);
	UObject* CreateObjectInstance(UClass* ObjectClass, const FTransform& SpawnTransform);
	UObject* CreateTemporaryObject(UClass* ObjectClass, const FTransform& SpawnTransform, UObject* RequestContext);
	int32 ExpandPool(FTTDObjectPoolRuntimeState& PoolState, int32 RequestedCount);
	void PrepareObjectForAcquire(UObject* Object, const FTransform& SpawnTransform) const;
	void PrepareObjectForPool(UObject* Object) const;
	void DestroyObjectInstance(UObject* Object) const;
	int32 CountValidObjects(const TArray<TObjectPtr<UObject>>& Objects) const;
	void RemoveInvalidObjects(TArray<TObjectPtr<UObject>>& Objects) const;
};
