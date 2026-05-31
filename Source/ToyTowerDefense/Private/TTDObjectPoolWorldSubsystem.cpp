#include "TTDObjectPoolWorldSubsystem.h"

#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "TTDDeveloperSettings.h"
#include "TTDGameplayTags.h"
#include "TTDPooledObjectInterface.h"

DEFINE_LOG_CATEGORY_STATIC(LogTTDObjectPool, Log, All);

namespace
{
	bool IsUsablePoolObject(const UObject* Object)
	{
		if (!IsValid(Object))
		{
			return false;
		}

		const AActor* Actor = Cast<AActor>(Object);
		return !Actor || !Actor->IsActorBeingDestroyed();
	}

	FTTDObjectPoolDefinition NormalizeDefinition(FTTDObjectPoolDefinition Definition)
	{
		Definition.InitialSize = FMath::Max(0, Definition.InitialSize);
		Definition.MaxSize = FMath::Max(0, Definition.MaxSize);
		Definition.ExpandBy = FMath::Max(1, Definition.ExpandBy);
		Definition.InitialSize = FMath::Min(Definition.InitialSize, Definition.MaxSize);
		return Definition;
	}
}

void UTTDObjectPoolWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadPoolDefinitionsFromSettings();
}

void UTTDObjectPoolWorldSubsystem::Deinitialize()
{
	for (TPair<TObjectPtr<UClass>, FTTDObjectPoolRuntimeState>& Pair : PoolStates)
	{
		for (TObjectPtr<UObject>& Object : Pair.Value.AvailableObjects)
		{
			DestroyObjectInstance(Object.Get());
		}

		for (TObjectPtr<UObject>& Object : Pair.Value.ActiveObjects)
		{
			DestroyObjectInstance(Object.Get());
		}
	}

	PoolStates.Reset();
	Super::Deinitialize();
}

void UTTDObjectPoolWorldSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	for (TPair<TObjectPtr<UClass>, FTTDObjectPoolRuntimeState>& Pair : PoolStates)
	{
		if (Pair.Value.Definition.bPrewarmOnWorldBeginPlay && Pair.Value.Definition.InitialSize > 0)
		{
			ExpandPool(Pair.Value, Pair.Value.Definition.InitialSize);
		}
	}
}

UObject* UTTDObjectPoolWorldSubsystem::AcquireObject(
	const TSubclassOf<UObject> ObjectClass,
	const FTransform& SpawnTransform,
	UObject* RequestContext,
	bool& bFromPool)
{
	bFromPool = false;
	UClass* Class = ObjectClass.Get();
	if (!IsPoolableClass(Class))
	{
		UE_LOG(LogTTDObjectPool, Warning, TEXT("AcquireObject rejected invalid or non-poolable class."));
		return nullptr;
	}

	if (FTTDObjectPoolRuntimeState* PoolState = FindPoolState(Class))
	{
		return AcquireFromPool(*PoolState, SpawnTransform, RequestContext, bFromPool);
	}

	return CreateTemporaryObject(Class, SpawnTransform, RequestContext);
}

AActor* UTTDObjectPoolWorldSubsystem::AcquireActor(
	const TSubclassOf<AActor> ActorClass,
	const FTransform& SpawnTransform,
	UObject* RequestContext,
	bool& bFromPool)
{
	return Cast<AActor>(AcquireObject(ActorClass, SpawnTransform, RequestContext, bFromPool));
}

bool UTTDObjectPoolWorldSubsystem::ReleaseObject(UObject* Object)
{
	if (!IsUsablePoolObject(Object))
	{
		return false;
	}

	UClass* ObjectClass = Object->GetClass();
	if (FTTDObjectPoolRuntimeState* PoolState = FindPoolState(ObjectClass))
	{
		const int32 ActiveIndex = PoolState->ActiveObjects.IndexOfByPredicate(
			[Object](const TObjectPtr<UObject>& Candidate)
			{
				return Candidate.Get() == Object;
			});

		if (ActiveIndex == INDEX_NONE)
		{
			UE_LOG(LogTTDObjectPool, Warning, TEXT("ReleaseObject rejected unmanaged active object of configured class %s."), *GetNameSafe(ObjectClass));
			return false;
		}

		PoolState->ActiveObjects.RemoveAtSwap(ActiveIndex);
		ITTDPooledObjectInterface::Execute_OnReleaseToPool(Object);
		PrepareObjectForPool(Object);
		PoolState->AvailableObjects.Add(Object);
		return true;
	}

	if (!ObjectClass->ImplementsInterface(UTTDPooledObjectInterface::StaticClass()))
	{
		return false;
	}

	ITTDPooledObjectInterface::Execute_OnReleaseToPool(Object);
	DestroyObjectInstance(Object);
	return true;
}

int32 UTTDObjectPoolWorldSubsystem::PrewarmPool(const TSubclassOf<UObject> ObjectClass, const int32 Count)
{
	if (Count <= 0)
	{
		return 0;
	}

	if (FTTDObjectPoolRuntimeState* PoolState = FindPoolState(ObjectClass.Get()))
	{
		return ExpandPool(*PoolState, Count);
	}

	UE_LOG(LogTTDObjectPool, Warning, TEXT("PrewarmPool rejected unconfigured class %s."), *GetNameSafe(ObjectClass.Get()));
	return 0;
}

FTTDObjectPoolStats UTTDObjectPoolWorldSubsystem::GetPoolStats(const TSubclassOf<UObject> ObjectClass) const
{
	FTTDObjectPoolStats Stats;
	Stats.ObjectClass = ObjectClass;

	if (const FTTDObjectPoolRuntimeState* PoolState = FindPoolState(ObjectClass.Get()))
	{
		Stats.bConfigured = true;
		Stats.AvailableCount = CountValidObjects(PoolState->AvailableObjects);
		Stats.ActiveCount = CountValidObjects(PoolState->ActiveObjects);
		Stats.TotalPooledCount = Stats.AvailableCount + Stats.ActiveCount;
		Stats.MaxSize = PoolState->Definition.MaxSize;
	}

	return Stats;
}

void UTTDObjectPoolWorldSubsystem::LoadPoolDefinitionsFromSettings()
{
	PoolStates.Reset();

	const UTTDDeveloperSettings* Settings = GetDefault<UTTDDeveloperSettings>();
	const UDataTable* DataTable = Settings ? Settings->GetDataTableForTag(TAG_TTD_DataTable_ObjectPool_Definitions) : nullptr;
	if (!DataTable)
	{
		return;
	}

	if (DataTable->GetRowStruct() != FTTDObjectPoolDefinition::StaticStruct())
	{
		UE_LOG(LogTTDObjectPool, Warning, TEXT("Object pool DataTable %s has incompatible row struct."), *GetNameSafe(DataTable));
		return;
	}

	for (const TPair<FName, uint8*>& RowPair : DataTable->GetRowMap())
	{
		const FTTDObjectPoolDefinition* Row = reinterpret_cast<const FTTDObjectPoolDefinition*>(RowPair.Value);
		if (!Row)
		{
			continue;
		}

		FTTDObjectPoolDefinition Definition = NormalizeDefinition(*Row);
		UClass* ObjectClass = Definition.ObjectClass.LoadSynchronous();
		if (!IsPoolableClass(ObjectClass))
		{
			UE_LOG(LogTTDObjectPool, Warning, TEXT("Object pool row %s skipped invalid or non-poolable class."), *RowPair.Key.ToString());
			continue;
		}

		FTTDObjectPoolRuntimeState& PoolState = PoolStates.FindOrAdd(ObjectClass);
		PoolState.ObjectClass = ObjectClass;
		PoolState.Definition = Definition;
		PoolState.AvailableObjects.Reset();
		PoolState.ActiveObjects.Reset();
	}
}

bool UTTDObjectPoolWorldSubsystem::IsPoolableClass(const UClass* ObjectClass) const
{
	if (!ObjectClass || !ObjectClass->IsChildOf(UObject::StaticClass()))
	{
		return false;
	}

	if (ObjectClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists))
	{
		return false;
	}

	return ObjectClass->ImplementsInterface(UTTDPooledObjectInterface::StaticClass());
}

FTTDObjectPoolRuntimeState* UTTDObjectPoolWorldSubsystem::FindPoolState(const UClass* ObjectClass)
{
	if (!ObjectClass)
	{
		return nullptr;
	}

	const TObjectPtr<UClass> Key = const_cast<UClass*>(ObjectClass);
	return PoolStates.Find(Key);
}

const FTTDObjectPoolRuntimeState* UTTDObjectPoolWorldSubsystem::FindPoolState(const UClass* ObjectClass) const
{
	if (!ObjectClass)
	{
		return nullptr;
	}

	const TObjectPtr<UClass> Key = const_cast<UClass*>(ObjectClass);
	return PoolStates.Find(Key);
}

UObject* UTTDObjectPoolWorldSubsystem::AcquireFromPool(
	FTTDObjectPoolRuntimeState& PoolState,
	const FTransform& SpawnTransform,
	UObject* RequestContext,
	bool& bFromPool)
{
	RemoveInvalidObjects(PoolState.AvailableObjects);
	RemoveInvalidObjects(PoolState.ActiveObjects);

	if (PoolState.AvailableObjects.IsEmpty())
	{
		ExpandPool(PoolState, PoolState.Definition.ExpandBy);
	}

	if (PoolState.AvailableObjects.IsEmpty())
	{
		UE_LOG(LogTTDObjectPool, Warning, TEXT("Object pool for %s reached max size %d."), *GetNameSafe(PoolState.ObjectClass.Get()), PoolState.Definition.MaxSize);
		return nullptr;
	}

	UObject* Object = PoolState.AvailableObjects.Pop().Get();
	if (!IsUsablePoolObject(Object))
	{
		return AcquireFromPool(PoolState, SpawnTransform, RequestContext, bFromPool);
	}

	PoolState.ActiveObjects.Add(Object);
	bFromPool = true;
	PrepareObjectForAcquire(Object, SpawnTransform);
	ITTDPooledObjectInterface::Execute_OnAcquireFromPool(Object, SpawnTransform, RequestContext);
	return Object;
}

UObject* UTTDObjectPoolWorldSubsystem::CreatePooledInstance(UClass* ObjectClass)
{
	UObject* Object = CreateObjectInstance(ObjectClass, FTransform::Identity);
	if (!Object)
	{
		return nullptr;
	}

	ITTDPooledObjectInterface::Execute_OnPoolCreated(Object, this);
	PrepareObjectForPool(Object);
	return Object;
}

UObject* UTTDObjectPoolWorldSubsystem::CreateObjectInstance(UClass* ObjectClass, const FTransform& SpawnTransform)
{
	if (!ObjectClass)
	{
		return nullptr;
	}

	if (ObjectClass->IsChildOf(AActor::StaticClass()))
	{
		UWorld* World = GetWorld();
		if (!World)
		{
			return nullptr;
		}

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		return World->SpawnActor<AActor>(ObjectClass, SpawnTransform, SpawnParameters);
	}

	return NewObject<UObject>(this, ObjectClass);
}

UObject* UTTDObjectPoolWorldSubsystem::CreateTemporaryObject(UClass* ObjectClass, const FTransform& SpawnTransform, UObject* RequestContext)
{
	UObject* Object = CreateObjectInstance(ObjectClass, SpawnTransform);
	if (!Object)
	{
		return nullptr;
	}

	PrepareObjectForAcquire(Object, SpawnTransform);
	ITTDPooledObjectInterface::Execute_OnAcquireFromPool(Object, SpawnTransform, RequestContext);
	return Object;
}

int32 UTTDObjectPoolWorldSubsystem::ExpandPool(FTTDObjectPoolRuntimeState& PoolState, const int32 RequestedCount)
{
	if (!PoolState.ObjectClass || RequestedCount <= 0)
	{
		return 0;
	}

	RemoveInvalidObjects(PoolState.AvailableObjects);
	RemoveInvalidObjects(PoolState.ActiveObjects);

	const int32 CurrentTotal = PoolState.AvailableObjects.Num() + PoolState.ActiveObjects.Num();
	const int32 RemainingCapacity = PoolState.Definition.MaxSize - CurrentTotal;
	const int32 CreateCount = FMath::Min(RequestedCount, RemainingCapacity);
	if (CreateCount <= 0)
	{
		return 0;
	}

	int32 CreatedCount = 0;
	for (int32 Index = 0; Index < CreateCount; ++Index)
	{
		if (UObject* Object = CreatePooledInstance(PoolState.ObjectClass.Get()))
		{
			PoolState.AvailableObjects.Add(Object);
			++CreatedCount;
		}
	}

	return CreatedCount;
}

void UTTDObjectPoolWorldSubsystem::PrepareObjectForAcquire(UObject* Object, const FTransform& SpawnTransform) const
{
	if (AActor* Actor = Cast<AActor>(Object))
	{
		Actor->SetActorTransform(SpawnTransform, false, nullptr, ETeleportType::TeleportPhysics);
		Actor->SetActorHiddenInGame(false);
		Actor->SetActorEnableCollision(true);
		Actor->SetActorTickEnabled(true);
	}
}

void UTTDObjectPoolWorldSubsystem::PrepareObjectForPool(UObject* Object) const
{
	if (AActor* Actor = Cast<AActor>(Object))
	{
		Actor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		Actor->SetActorHiddenInGame(true);
		Actor->SetActorEnableCollision(false);
		Actor->SetActorTickEnabled(false);
	}
}

void UTTDObjectPoolWorldSubsystem::DestroyObjectInstance(UObject* Object) const
{
	if (AActor* Actor = Cast<AActor>(Object))
	{
		if (IsValid(Actor) && !Actor->IsActorBeingDestroyed())
		{
			Actor->Destroy();
		}
	}
}

int32 UTTDObjectPoolWorldSubsystem::CountValidObjects(const TArray<TObjectPtr<UObject>>& Objects) const
{
	int32 Count = 0;
	for (const TObjectPtr<UObject>& Object : Objects)
	{
		if (IsUsablePoolObject(Object.Get()))
		{
			++Count;
		}
	}
	return Count;
}

void UTTDObjectPoolWorldSubsystem::RemoveInvalidObjects(TArray<TObjectPtr<UObject>>& Objects) const
{
	Objects.RemoveAll(
		[](const TObjectPtr<UObject>& Object)
		{
			return !IsUsablePoolObject(Object.Get());
		});
}
