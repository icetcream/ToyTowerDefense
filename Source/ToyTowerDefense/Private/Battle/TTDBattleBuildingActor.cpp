#include "Battle/TTDBattleBuildingActor.h"

#include "Battle/TTDBattleEnemyActor.h"
#include "Battle/TTDBattleProjectileActor.h"
#include "Battle/TTDBuildSlotActor.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "TTDBattleWorldSubsystem.h"
#include "TTDObjectPoolWorldSubsystem.h"
#include "UObject/ConstructorHelpers.h"

ATTDBattleBuildingActor::ATTDBattleBuildingActor()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(SceneRoot);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	VisualMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	VisualMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	VisualMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 50.0f));
	VisualMesh->SetRelativeScale3D(FVector(0.8f, 0.8f, 1.0f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> BuildingMeshAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (BuildingMeshAsset.Succeeded())
	{
		VisualMesh->SetStaticMesh(BuildingMeshAsset.Object);
	}
}

void ATTDBattleBuildingActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyVisualMaterial();
}

void ATTDBattleBuildingActor::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	TryAttack(DeltaSeconds);
}

void ATTDBattleBuildingActor::InitializeBuilding(
	const FName InBuildingId,
	const FTTDBuildingDefinition& Definition,
	const FTTDBattleBuildingRuntimeStats& InRuntimeStats,
	const TArray<FTTDNameStack>& InConstructionPartCosts,
	ATTDBuildSlotActor* InOwningSlot)
{
	BuildingId = InBuildingId;
	DisplayName = Definition.DisplayName;
	AttackMode = Definition.AttackMode;
	RuntimeStats = InRuntimeStats;
	ProjectileClass = Definition.ProjectileClass;
	ConstructionPartCosts = InConstructionPartCosts;
	UpgradePartCosts.Reset();
	OwningSlot = InOwningSlot;
	CurrentHealth = FMath::Max(1.0f, RuntimeStats.MaxHealth);
	AttackAccumulator = RuntimeStats.AttackInterval;
	BuildingLevel = 1;
	RefreshVisualState();
}

void ATTDBattleBuildingActor::ApplyRuntimeStats(const FTTDBattleBuildingRuntimeStats& InRuntimeStats)
{
	const float OldMaxHealth = FMath::Max(1.0f, RuntimeStats.MaxHealth);
	const float HealthPercent = OldMaxHealth > 0.0f ? FMath::Clamp(CurrentHealth / OldMaxHealth, 0.0f, 1.0f) : 1.0f;
	RuntimeStats = InRuntimeStats;
	RuntimeStats.MaxHealth = FMath::Max(1.0f, RuntimeStats.MaxHealth);
	RuntimeStats.AttackDamage = FMath::Max(0.0f, RuntimeStats.AttackDamage);
	RuntimeStats.AttackRange = FMath::Max(0.0f, RuntimeStats.AttackRange);
	RuntimeStats.AttackInterval = FMath::Max(0.01f, RuntimeStats.AttackInterval);
	RuntimeStats.ProjectileSpeed = FMath::Max(1.0f, RuntimeStats.ProjectileSpeed);
	RuntimeStats.MoveSpeed = FMath::Max(0.0f, RuntimeStats.MoveSpeed);
	CurrentHealth = FMath::Max(1.0f, RuntimeStats.MaxHealth * HealthPercent);
	AttackAccumulator = FMath::Min(AttackAccumulator, RuntimeStats.AttackInterval);
}

void ATTDBattleBuildingActor::IncrementLevel()
{
	++BuildingLevel;
}

void ATTDBattleBuildingActor::AddUpgradePartCost(const FName PartId, const int32 Count)
{
	if (PartId.IsNone() || Count <= 0)
	{
		return;
	}

	for (FTTDNameStack& ExistingCost : UpgradePartCosts)
	{
		if (ExistingCost.Id == PartId)
		{
			ExistingCost.Count += Count;
			return;
		}
	}

	UpgradePartCosts.Add(FTTDNameStack(PartId, Count));
}

FVector ATTDBattleBuildingActor::GetBattleTargetLocation_Implementation() const
{
	return GetActorLocation();
}

bool ATTDBattleBuildingActor::IsBattleTargetAlive_Implementation() const
{
	return CurrentHealth > 0.0f;
}

void ATTDBattleBuildingActor::ApplyBattleDamage_Implementation(const float DamageAmount, UObject* DamageSourceObject)
{
	if (DamageAmount <= 0.0f || CurrentHealth <= 0.0f)
	{
		return;
	}

	CurrentHealth = FMath::Max(0.0f, CurrentHealth - DamageAmount);
	if (CurrentHealth <= 0.0f)
	{
		if (UTTDBattleWorldSubsystem* BattleSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UTTDBattleWorldSubsystem>() : nullptr)
		{
			BattleSubsystem->HandleBuildingDestroyed(this);
		}
	}
}

void ATTDBattleBuildingActor::OnPoolCreated_Implementation(UObject* PoolOwner)
{
	SetActorTickEnabled(false);
}

void ATTDBattleBuildingActor::OnAcquireFromPool_Implementation(const FTransform& SpawnTransform, UObject* RequestContext)
{
	SetActorTickEnabled(true);
	RefreshVisualState();
}

void ATTDBattleBuildingActor::OnReleaseToPool_Implementation()
{
	SetActorTickEnabled(false);
	BuildingId = NAME_None;
	DisplayName = FText::GetEmpty();
	CurrentHealth = 0.0f;
	OwningSlot.Reset();
	ConstructionPartCosts.Reset();
	UpgradePartCosts.Reset();
	AttackAccumulator = 0.0f;
	BuildingLevel = 1;
}

void ATTDBattleBuildingActor::TryAttack(const float DeltaSeconds)
{
	if (CurrentHealth <= 0.0f || DeltaSeconds <= 0.0f)
	{
		return;
	}

	UTTDBattleWorldSubsystem* BattleSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UTTDBattleWorldSubsystem>() : nullptr;
	if (!BattleSubsystem || !BattleSubsystem->IsBattleRunning())
	{
		return;
	}

	AttackAccumulator += DeltaSeconds;
	const float SafeInterval = FMath::Max(0.01f, RuntimeStats.AttackInterval);
	if (AttackAccumulator < SafeInterval)
	{
		return;
	}

	ATTDBattleEnemyActor* Target = BattleSubsystem->FindNearestEnemy(GetActorLocation(), RuntimeStats.AttackRange);
	if (!Target)
	{
		return;
	}

	AttackAccumulator = 0.0f;
	if (AttackMode == ETTDBuildingAttackMode::Projectile)
	{
		FireProjectile(Target);
	}
	else
	{
		FireInstantDamage(Target);
	}
}

void ATTDBattleBuildingActor::FireInstantDamage(ATTDBattleEnemyActor* Target)
{
	if (Target)
	{
		Target->ApplyDamage(RuntimeStats.AttackDamage, this);
	}
}

void ATTDBattleBuildingActor::FireProjectile(ATTDBattleEnemyActor* Target)
{
	if (!Target)
	{
		return;
	}

	UClass* ProjectileActorClass = ProjectileClass.LoadSynchronous();
	if (!ProjectileActorClass)
	{
		ProjectileActorClass = ATTDBattleProjectileActor::StaticClass();
	}

	UTTDBattleWorldSubsystem* BattleSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UTTDBattleWorldSubsystem>() : nullptr;
	if (!BattleSubsystem || !BattleSubsystem->IsBattleRunning())
	{
		return;
	}

	bool bFromPool = false;
	AActor* ProjectileActor = nullptr;
	if (UTTDObjectPoolWorldSubsystem* PoolSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UTTDObjectPoolWorldSubsystem>() : nullptr)
	{
		ProjectileActor = PoolSubsystem->AcquireActor(ProjectileActorClass, GetActorTransform(), this, bFromPool);
	}

	if (ATTDBattleProjectileActor* Projectile = Cast<ATTDBattleProjectileActor>(ProjectileActor))
	{
		Projectile->InitializeProjectile(Target, RuntimeStats.AttackDamage, RuntimeStats.ProjectileSpeed, this, BattleSubsystem->GetBattleInstanceId());
		BattleSubsystem->RegisterProjectile(Projectile, BattleSubsystem->GetBattleInstanceId());
	}
}

void ATTDBattleBuildingActor::RefreshVisualState()
{
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);

	if (VisualMesh)
	{
		VisualMesh->SetVisibility(true, true);
		VisualMesh->SetHiddenInGame(false, true);
		ApplyVisualMaterial();
	}
}

void ATTDBattleBuildingActor::ApplyVisualMaterial()
{
	if (VisualMesh && VisualMaterial)
	{
		VisualMesh->SetMaterial(0, VisualMaterial);
	}
}
