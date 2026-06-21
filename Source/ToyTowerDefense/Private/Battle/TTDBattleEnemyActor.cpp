#include "Battle/TTDBattleEnemyActor.h"

#include "Battle/TTDBattleTargetInterface.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "TTDBattleWorldSubsystem.h"
#include "UObject/ConstructorHelpers.h"

ATTDBattleEnemyActor::ATTDBattleEnemyActor()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(SceneRoot);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VisualMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 50.0f));
	VisualMesh->SetRelativeScale3D(FVector(0.55f, 0.55f, 1.0f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> EnemyMeshAsset(TEXT("/Engine/BasicShapes/Cone.Cone"));
	if (EnemyMeshAsset.Succeeded())
	{
		VisualMesh->SetStaticMesh(EnemyMeshAsset.Object);
	}
}

void ATTDBattleEnemyActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyVisualMaterial();
}

void ATTDBattleEnemyActor::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!IsAlive())
	{
		return;
	}

	if (UTTDBattleWorldSubsystem* BattleSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UTTDBattleWorldSubsystem>() : nullptr)
	{
		MoveAndAttack(DeltaSeconds, *BattleSubsystem);
	}
}

void ATTDBattleEnemyActor::InitializeEnemy(const FTTDEnemyDefinition& Definition)
{
	EnemyId = Definition.EnemyId;
	MaxHealth = FMath::Max(1.0f, Definition.MaxHealth);
	CurrentHealth = MaxHealth;
	MoveSpeed = FMath::Max(0.0f, Definition.MoveSpeed);
	AttackDamage = FMath::Max(0.0f, Definition.AttackDamage);
	AttackRange = FMath::Max(0.0f, Definition.AttackRange);
	AttackInterval = FMath::Max(0.01f, Definition.AttackInterval);
	ProgressWeight = FMath::Max(0.0f, Definition.ProgressWeight);
	CurrencyDrop = FMath::Max(0, Definition.CurrencyDrop);
	CurrencyDropChance = FMath::Clamp(Definition.CurrencyDropChance, 0.0f, 1.0f);
	AttackAccumulator = 0.0f;
	ApplyVisualMaterial();
}

void ATTDBattleEnemyActor::ApplyDamage(const float Damage, UObject* DamageSource)
{
	if (Damage <= 0.0f || !IsAlive())
	{
		return;
	}

	CurrentHealth = FMath::Max(0.0f, CurrentHealth - Damage);
	if (!IsAlive())
	{
		if (UTTDBattleWorldSubsystem* BattleSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UTTDBattleWorldSubsystem>() : nullptr)
		{
			BattleSubsystem->HandleEnemyKilled(this);
		}
	}
}

void ATTDBattleEnemyActor::OnPoolCreated_Implementation(UObject* PoolOwner)
{
	SetActorTickEnabled(false);
}

void ATTDBattleEnemyActor::OnAcquireFromPool_Implementation(const FTransform& SpawnTransform, UObject* RequestContext)
{
	AttackAccumulator = 0.0f;
	SetActorTickEnabled(true);
	ApplyVisualMaterial();
}

void ATTDBattleEnemyActor::OnReleaseToPool_Implementation()
{
	SetActorTickEnabled(false);
	CurrentHealth = 0.0f;
}

void ATTDBattleEnemyActor::MoveAndAttack(const float DeltaSeconds, UTTDBattleWorldSubsystem& BattleSubsystem)
{
	UObject* TargetObject = BattleSubsystem.FindNearestBattleTarget(GetActorLocation());
	if (!TargetObject || !TargetObject->GetClass()->ImplementsInterface(UTTDBattleTargetInterface::StaticClass()))
	{
		return;
	}

	const ITTDBattleTargetInterface* NativeTarget = Cast<ITTDBattleTargetInterface>(TargetObject);
	const FVector TargetLocation = NativeTarget
		? NativeTarget->GetBattleTargetLocation_Implementation()
		: ITTDBattleTargetInterface::Execute_GetBattleTargetLocation(TargetObject);
	const FVector ToTarget = TargetLocation - GetActorLocation();
	const float Distance = ToTarget.Size2D();

	if (Distance > AttackRange)
	{
		const FVector Direction = ToTarget.GetSafeNormal2D();
		AddActorWorldOffset(Direction * MoveSpeed * DeltaSeconds, true);
		return;
	}

	AttackAccumulator += DeltaSeconds;
	if (AttackAccumulator >= AttackInterval)
	{
		AttackAccumulator = 0.0f;
		if (ITTDBattleTargetInterface* MutableNativeTarget = Cast<ITTDBattleTargetInterface>(TargetObject))
		{
			MutableNativeTarget->ApplyBattleDamage_Implementation(AttackDamage, this);
		}
		else
		{
			ITTDBattleTargetInterface::Execute_ApplyBattleDamage(TargetObject, AttackDamage, this);
		}
	}
}

void ATTDBattleEnemyActor::ApplyVisualMaterial()
{
	if (VisualMesh && VisualMaterial)
	{
		VisualMesh->SetMaterial(0, VisualMaterial);
	}
}
