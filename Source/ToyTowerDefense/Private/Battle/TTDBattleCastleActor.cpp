#include "Battle/TTDBattleCastleActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "TTDBattleWorldSubsystem.h"
#include "UObject/ConstructorHelpers.h"

ATTDBattleCastleActor::ATTDBattleCastleActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(SceneRoot);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VisualMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 75.0f));
	VisualMesh->SetRelativeScale3D(FVector(2.0f, 2.0f, 1.5f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CastleMeshAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CastleMeshAsset.Succeeded())
	{
		VisualMesh->SetStaticMesh(CastleMeshAsset.Object);
	}

	CurrentHealth = MaxHealth;
}

void ATTDBattleCastleActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyVisualMaterial();
}

void ATTDBattleCastleActor::InitializeCastle(const float InMaxHealth)
{
	MaxHealth = FMath::Max(1.0f, InMaxHealth);
	CurrentHealth = MaxHealth;
	ApplyVisualMaterial();
}

FVector ATTDBattleCastleActor::GetBattleTargetLocation_Implementation() const
{
	return GetActorLocation();
}

bool ATTDBattleCastleActor::IsBattleTargetAlive_Implementation() const
{
	return CurrentHealth > 0.0f;
}

void ATTDBattleCastleActor::ApplyBattleDamage_Implementation(const float Damage, UObject* DamageSource)
{
	if (Damage <= 0.0f || CurrentHealth <= 0.0f)
	{
		return;
	}

	CurrentHealth = FMath::Max(0.0f, CurrentHealth - Damage);
	if (CurrentHealth <= 0.0f)
	{
		if (UTTDBattleWorldSubsystem* BattleSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UTTDBattleWorldSubsystem>() : nullptr)
		{
			BattleSubsystem->HandleCastleDestroyed(this);
		}
	}
}

void ATTDBattleCastleActor::ApplyVisualMaterial()
{
	if (VisualMesh && VisualMaterial)
	{
		VisualMesh->SetMaterial(0, VisualMaterial);
	}
}
