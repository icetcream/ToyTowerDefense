#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"
#include "TTDPooledObjectInterface.h"
#include "TTDObjectPoolTestTypes.generated.h"

UCLASS()
class UTTDTestPooledObject : public UObject, public ITTDPooledObjectInterface
{
	GENERATED_BODY()

public:
	int32 CreatedCount = 0;
	int32 AcquiredCount = 0;
	int32 ReleasedCount = 0;
	TObjectPtr<UObject> LastRequestContext;

	virtual void OnPoolCreated_Implementation(UObject* PoolOwner) override
	{
		++CreatedCount;
	}

	virtual void OnAcquireFromPool_Implementation(const FTransform& SpawnTransform, UObject* RequestContext) override
	{
		++AcquiredCount;
		LastRequestContext = RequestContext;
	}

	virtual void OnReleaseToPool_Implementation() override
	{
		++ReleasedCount;
	}
};

UCLASS()
class UTTDTestTemporaryPooledObject : public UTTDTestPooledObject
{
	GENERATED_BODY()
};

UCLASS()
class UTTDTestNonPooledObject : public UObject
{
	GENERATED_BODY()
};

UCLASS()
class ATTDTestPooledActor : public AActor, public ITTDPooledObjectInterface
{
	GENERATED_BODY()

public:
	ATTDTestPooledActor()
	{
		RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	}

	int32 CreatedCount = 0;
	int32 AcquiredCount = 0;
	int32 ReleasedCount = 0;

	virtual void OnPoolCreated_Implementation(UObject* PoolOwner) override
	{
		++CreatedCount;
	}

	virtual void OnAcquireFromPool_Implementation(const FTransform& SpawnTransform, UObject* RequestContext) override
	{
		++AcquiredCount;
	}

	virtual void OnReleaseToPool_Implementation() override
	{
		++ReleasedCount;
	}
};
