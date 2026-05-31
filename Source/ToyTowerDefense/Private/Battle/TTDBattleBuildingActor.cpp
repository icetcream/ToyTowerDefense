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
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
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
	ATTDBuildSlotActor* InOwningSlot)
{
	BuildingId = InBuildingId;
	AttackMode = Definition.AttackMode;
	RuntimeStats = InRuntimeStats;
	ProjectileClass = Definition.ProjectileClass;
	OwningSlot = InOwningSlot;
	CurrentHealth = FMath::Max(1.0f, RuntimeStats.MaxHealth);
	AttackAccumulator = RuntimeStats.AttackInterval;
	RefreshVisualState();
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
	CurrentHealth = 0.0f;
	OwningSlot.Reset();
	AttackAccumulator = 0.0f;
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
