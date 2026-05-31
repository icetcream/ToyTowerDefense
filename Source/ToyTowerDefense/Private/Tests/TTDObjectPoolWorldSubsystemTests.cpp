#include "Misc/AutomationTest.h"

#include "Engine/DataTable.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Tests/TTDObjectPoolTestTypes.h"
#include "TTDDeveloperSettings.h"
#include "TTDGameplayTags.h"
#include "TTDObjectPoolTypes.h"
#include "TTDObjectPoolWorldSubsystem.h"
#include "UObject/StrongObjectPtr.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	UDataTable* CreateObjectPoolDataTable()
	{
		UDataTable* DataTable = NewObject<UDataTable>();
		DataTable->RowStruct = FTTDObjectPoolDefinition::StaticStruct();

		FTTDObjectPoolDefinition ObjectDefinition;
		ObjectDefinition.ObjectClass = UTTDTestPooledObject::StaticClass();
		ObjectDefinition.InitialSize = 1;
		ObjectDefinition.MaxSize = 2;
		ObjectDefinition.ExpandBy = 1;
		ObjectDefinition.bPrewarmOnWorldBeginPlay = true;
		DataTable->AddRow(TEXT("TestObject"), ObjectDefinition);

		FTTDObjectPoolDefinition ActorDefinition;
		ActorDefinition.ObjectClass = ATTDTestPooledActor::StaticClass();
		ActorDefinition.InitialSize = 1;
		ActorDefinition.MaxSize = 1;
		ActorDefinition.ExpandBy = 1;
		ActorDefinition.bPrewarmOnWorldBeginPlay = true;
		DataTable->AddRow(TEXT("TestActor"), ActorDefinition);

		return DataTable;
	}

	UWorld* CreateObjectPoolTestWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, FName(TEXT("TTDObjectPoolTestWorld")));
		if (!World)
		{
			return nullptr;
		}

		FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
		WorldContext.SetCurrentWorld(World);
		return World;
	}

	void DestroyObjectPoolTestWorld(UWorld* World)
	{
		if (!World)
		{
			return;
		}

		World->DestroyWorld(false);
		if (GEngine)
		{
			GEngine->DestroyWorldContext(World);
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTTDObjectPoolWorldSubsystemTests,
	"ToyTowerDefense.ObjectPool.WorldSubsystem",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTTDObjectPoolWorldSubsystemTests::RunTest(const FString& Parameters)
{
	UTTDDeveloperSettings* Settings = GetMutableDefault<UTTDDeveloperSettings>();
	const TMap<FGameplayTag, TSoftObjectPtr<UDataTable>> OriginalDataTables = Settings->GameplayTagDataTables;
	const TStrongObjectPtr<UDataTable> ObjectPoolDataTable(CreateObjectPoolDataTable());
	Settings->GameplayTagDataTables.Add(TAG_TTD_DataTable_ObjectPool_Definitions, ObjectPoolDataTable.Get());

	UWorld* World = CreateObjectPoolTestWorld();
	if (!TestNotNull(TEXT("test world is created"), World))
	{
		Settings->GameplayTagDataTables = OriginalDataTables;
		return false;
	}

	UTTDObjectPoolWorldSubsystem* PoolSubsystem = World->GetSubsystem<UTTDObjectPoolWorldSubsystem>();
	if (!TestNotNull(TEXT("object pool world subsystem is available"), PoolSubsystem))
	{
		DestroyObjectPoolTestWorld(World);
		Settings->GameplayTagDataTables = OriginalDataTables;
		return false;
	}

	TestEqual(TEXT("configured object class can be prewarmed"), PoolSubsystem->PrewarmPool(UTTDTestPooledObject::StaticClass(), 1), 1);
	FTTDObjectPoolStats Stats = PoolSubsystem->GetPoolStats(UTTDTestPooledObject::StaticClass());
	TestTrue(TEXT("configured object pool is reported as configured"), Stats.bConfigured);
	TestEqual(TEXT("prewarm creates one available object"), Stats.AvailableCount, 1);
	TestEqual(TEXT("prewarm creates no active objects"), Stats.ActiveCount, 0);

	bool bFromPool = false;
	UObject* FirstObject = PoolSubsystem->AcquireObject(UTTDTestPooledObject::StaticClass(), FTransform::Identity, nullptr, bFromPool);
	UTTDTestPooledObject* FirstPooledObject = Cast<UTTDTestPooledObject>(FirstObject);
	TestNotNull(TEXT("configured object can be acquired"), FirstPooledObject);
	TestTrue(TEXT("configured object is acquired from pool"), bFromPool);
	TestEqual(TEXT("pooled object receives acquire event"), FirstPooledObject ? FirstPooledObject->AcquiredCount : 0, 1);

	TestTrue(TEXT("pooled object can be released"), PoolSubsystem->ReleaseObject(FirstObject));
	Stats = PoolSubsystem->GetPoolStats(UTTDTestPooledObject::StaticClass());
	TestEqual(TEXT("release returns object to available list"), Stats.AvailableCount, 1);
	TestEqual(TEXT("release clears active list"), Stats.ActiveCount, 0);

	UObject* ReusedObject = PoolSubsystem->AcquireObject(UTTDTestPooledObject::StaticClass(), FTransform::Identity, nullptr, bFromPool);
	TestEqual(TEXT("released object is reused"), ReusedObject, FirstObject);

	UObject* ExpandedObject = PoolSubsystem->AcquireObject(UTTDTestPooledObject::StaticClass(), FTransform::Identity, nullptr, bFromPool);
	TestNotNull(TEXT("pool expands when capacity remains"), ExpandedObject);
	Stats = PoolSubsystem->GetPoolStats(UTTDTestPooledObject::StaticClass());
	TestEqual(TEXT("pool expansion reaches max pooled count"), Stats.TotalPooledCount, 2);

	UObject* OverflowObject = PoolSubsystem->AcquireObject(UTTDTestPooledObject::StaticClass(), FTransform::Identity, nullptr, bFromPool);
	TestNull(TEXT("configured pool refuses objects beyond max size"), OverflowObject);

	UObject* TemporaryObject = PoolSubsystem->AcquireObject(UTTDTestTemporaryPooledObject::StaticClass(), FTransform::Identity, nullptr, bFromPool);
	TestNotNull(TEXT("unconfigured poolable class can be created temporarily"), TemporaryObject);
	TestFalse(TEXT("temporary object is not acquired from pool"), bFromPool);
	TestFalse(TEXT("temporary class is not reported as configured"), PoolSubsystem->GetPoolStats(UTTDTestTemporaryPooledObject::StaticClass()).bConfigured);
	TestTrue(TEXT("temporary object can be released without entering a pool"), PoolSubsystem->ReleaseObject(TemporaryObject));

	UObject* NonPoolableObject = PoolSubsystem->AcquireObject(UTTDTestNonPooledObject::StaticClass(), FTransform::Identity, nullptr, bFromPool);
	TestNull(TEXT("non-interface class is rejected"), NonPoolableObject);

	TestEqual(TEXT("configured actor class can be prewarmed"), PoolSubsystem->PrewarmPool(ATTDTestPooledActor::StaticClass(), 1), 1);
	const FTransform ActorTransform(FRotator(0.0f, 45.0f, 0.0f), FVector(125.0f, 250.0f, 375.0f));
	AActor* Actor = PoolSubsystem->AcquireActor(ATTDTestPooledActor::StaticClass(), ActorTransform, nullptr, bFromPool);
	TestNotNull(TEXT("configured actor can be acquired"), Actor);
	TestTrue(TEXT("configured actor is acquired from pool"), bFromPool);
	if (Actor)
	{
		TestEqual(TEXT("actor acquire applies location"), Actor->GetActorLocation(), ActorTransform.GetLocation());
		TestTrue(TEXT("actor is visible when acquired"), !Actor->IsHidden());
		TestTrue(TEXT("actor can be released"), PoolSubsystem->ReleaseObject(Actor));
		TestTrue(TEXT("actor is hidden after release"), Actor->IsHidden());
		TestFalse(TEXT("actor collision is disabled after release"), Actor->GetActorEnableCollision());
	}

	DestroyObjectPoolTestWorld(World);
	Settings->GameplayTagDataTables = OriginalDataTables;
	return true;
}

#endif
