#include "Battle/TTDBattleSpawnPointActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

ATTDBattleSpawnPointActor::ATTDBattleSpawnPointActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(SceneRoot);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VisualMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 25.0f));
	VisualMesh->SetRelativeScale3D(FVector(0.5f, 0.5f, 0.5f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SpawnPointMeshAsset(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (SpawnPointMeshAsset.Succeeded())
	{
		VisualMesh->SetStaticMesh(SpawnPointMeshAsset.Object);
	}
}

void ATTDBattleSpawnPointActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyVisualMaterial();
}

void ATTDBattleSpawnPointActor::ConfigureSpawnPoint(const FName InSpawnGroup)
{
	SpawnGroup = InSpawnGroup;
	ApplyVisualMaterial();
}

void ATTDBattleSpawnPointActor::ApplyVisualMaterial()
{
	if (VisualMesh && VisualMaterial)
	{
		VisualMesh->SetMaterial(0, VisualMaterial);
	}
}
