#include "Battle/TTDBattleProjectileActor.h"

#include "Battle/TTDBattleEnemyActor.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "TTDBattleWorldSubsystem.h"
#include "TTDObjectPoolWorldSubsystem.h"
#include "UObject/ConstructorHelpers.h"

ATTDBattleProjectileActor::ATTDBattleProjectileActor()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(SceneRoot);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VisualMesh->SetRelativeScale3D(FVector(0.2f, 0.2f, 0.2f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> ProjectileMeshAsset(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (ProjectileMeshAsset.Succeeded())
	{
		VisualMesh->SetStaticMesh(ProjectileMeshAsset.Object);
	}
}

void ATTDBattleProjectileActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyVisualMaterial();
}

void ATTDBattleProjectileActor::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UTTDBattleWorldSubsystem* BattleSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UTTDBattleWorldSubsystem>() : nullptr;
	if (!BattleSubsystem || !BattleSubsystem->IsBattleInstanceCurrent(BattleInstanceId))
	{
		ReleaseSelf();
		return;
	}

	ATTDBattleEnemyActor* TargetActor = Target.Get();
	if (!TargetActor || !TargetActor->IsAlive())
	{
		ReleaseSelf();
		return;
	}

	const FVector ToTarget = TargetActor->GetActorLocation() - GetActorLocation();
	if (ToTarget.SizeSquared() <= FMath::Square(HitRadius))
	{
		TargetActor->ApplyDamage(Damage, DamageSource.Get());
		ReleaseSelf();
		return;
	}

	const FVector Direction = ToTarget.GetSafeNormal();
	AddActorWorldOffset(Direction * Speed * DeltaSeconds, true);
}

void ATTDBattleProjectileActor::InitializeProjectile(ATTDBattleEnemyActor* InTarget, const float InDamage, const float InSpeed, UObject* InDamageSource, const int32 InBattleInstanceId)
{
	Target = InTarget;
	Damage = FMath::Max(0.0f, InDamage);
	Speed = FMath::Max(1.0f, InSpeed);
	DamageSource = InDamageSource;
	BattleInstanceId = InBattleInstanceId;
	SetActorTickEnabled(true);
	ApplyVisualMaterial();
}

void ATTDBattleProjectileActor::OnPoolCreated_Implementation(UObject* PoolOwner)
{
	SetActorTickEnabled(false);
}

void ATTDBattleProjectileActor::OnAcquireFromPool_Implementation(const FTransform& SpawnTransform, UObject* RequestContext)
{
	SetActorTickEnabled(true);
	ApplyVisualMaterial();
}

void ATTDBattleProjectileActor::OnReleaseToPool_Implementation()
{
	SetActorTickEnabled(false);
	Target.Reset();
	DamageSource.Reset();
	Damage = 0.0f;
	BattleInstanceId = INDEX_NONE;
}

void ATTDBattleProjectileActor::ReleaseSelf()
{
	if (UTTDBattleWorldSubsystem* BattleSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UTTDBattleWorldSubsystem>() : nullptr)
	{
		BattleSubsystem->UnregisterProjectile(this);
	}

	if (UTTDObjectPoolWorldSubsystem* PoolSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UTTDObjectPoolWorldSubsystem>() : nullptr)
	{
		PoolSubsystem->ReleaseObject(this);
	}
	else
	{
		Destroy();
	}
}

void ATTDBattleProjectileActor::ApplyVisualMaterial()
{
	if (VisualMesh && VisualMaterial)
	{
		VisualMesh->SetMaterial(0, VisualMaterial);
	}
}
