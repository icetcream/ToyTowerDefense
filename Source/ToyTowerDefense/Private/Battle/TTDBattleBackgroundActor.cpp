#include "Battle/TTDBattleBackgroundActor.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

ATTDBattleBackgroundActor::ATTDBattleBackgroundActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	GroundMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GroundMesh"));
	GroundMesh->SetupAttachment(SceneRoot);

	NorthWallMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("NorthWallMesh"));
	NorthWallMesh->SetupAttachment(SceneRoot);

	SouthWallMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SouthWallMesh"));
	SouthWallMesh->SetupAttachment(SceneRoot);

	EastWallMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EastWallMesh"));
	EastWallMesh->SetupAttachment(SceneRoot);

	WestWallMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WestWallMesh"));
	WestWallMesh->SetupAttachment(SceneRoot);

	KeyLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("KeyLight"));
	KeyLight->SetupAttachment(SceneRoot);
	KeyLight->SetRelativeRotation(FRotator(-55.0f, -35.0f, 0.0f));
	KeyLight->SetIntensity(1.5f);
	KeyLight->SetCastShadows(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshAsset.Succeeded())
	{
		for (UStaticMeshComponent* MeshComponent : { GroundMesh.Get(), NorthWallMesh.Get(), SouthWallMesh.Get(), EastWallMesh.Get(), WestWallMesh.Get() })
		{
			if (MeshComponent)
			{
				MeshComponent->SetStaticMesh(CubeMeshAsset.Object);
			}
		}
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BaseMaterialAsset(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (BaseMaterialAsset.Succeeded())
	{
		BaseMaterial = BaseMaterialAsset.Object;
	}
}

void ATTDBattleBackgroundActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyDefinition();
}

void ATTDBattleBackgroundActor::ConfigureBackground(const FTTDBattleBackgroundDefinition& InDefinition)
{
	Definition = InDefinition;
	ApplyDefinition();
}

void ATTDBattleBackgroundActor::ApplyDefinition()
{
	const FVector2D HalfExtent(
		FMath::Max(100.0f, Definition.ArenaHalfExtent.X),
		FMath::Max(100.0f, Definition.ArenaHalfExtent.Y));
	const float GroundThickness = FMath::Max(1.0f, Definition.GroundThickness);
	const float WallHeight = FMath::Max(0.0f, Definition.WallHeight);
	const float WallThickness = FMath::Max(0.0f, Definition.WallThickness);

	ConfigureCubeMesh(GroundMesh);
	GroundMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -GroundThickness * 0.5f));
	GroundMesh->SetRelativeScale3D(FVector(HalfExtent.X / 50.0f, HalfExtent.Y / 50.0f, GroundThickness / 100.0f));
	ApplyMeshColor(GroundMesh, Definition.GroundColor);

	const bool bShowWalls = WallHeight > 0.0f && WallThickness > 0.0f;
	for (UStaticMeshComponent* WallMesh : { NorthWallMesh.Get(), SouthWallMesh.Get(), EastWallMesh.Get(), WestWallMesh.Get() })
	{
		ConfigureCubeMesh(WallMesh);
		WallMesh->SetVisibility(bShowWalls, true);
		WallMesh->SetHiddenInGame(!bShowWalls);
		ApplyMeshColor(WallMesh, Definition.WallColor);
	}

	if (bShowWalls)
	{
		NorthWallMesh->SetRelativeLocation(FVector(0.0f, HalfExtent.Y + WallThickness * 0.5f, WallHeight * 0.5f));
		NorthWallMesh->SetRelativeScale3D(FVector((HalfExtent.X + WallThickness) / 50.0f, WallThickness / 100.0f, WallHeight / 100.0f));

		SouthWallMesh->SetRelativeLocation(FVector(0.0f, -HalfExtent.Y - WallThickness * 0.5f, WallHeight * 0.5f));
		SouthWallMesh->SetRelativeScale3D(FVector((HalfExtent.X + WallThickness) / 50.0f, WallThickness / 100.0f, WallHeight / 100.0f));

		EastWallMesh->SetRelativeLocation(FVector(HalfExtent.X + WallThickness * 0.5f, 0.0f, WallHeight * 0.5f));
		EastWallMesh->SetRelativeScale3D(FVector(WallThickness / 100.0f, (HalfExtent.Y + WallThickness) / 50.0f, WallHeight / 100.0f));

		WestWallMesh->SetRelativeLocation(FVector(-HalfExtent.X - WallThickness * 0.5f, 0.0f, WallHeight * 0.5f));
		WestWallMesh->SetRelativeScale3D(FVector(WallThickness / 100.0f, (HalfExtent.Y + WallThickness) / 50.0f, WallHeight / 100.0f));
	}

	if (KeyLight)
	{
		KeyLight->SetLightColor(Definition.LightColor);
	}
}

void ATTDBattleBackgroundActor::ConfigureCubeMesh(UStaticMeshComponent* MeshComponent) const
{
	if (!MeshComponent)
	{
		return;
	}

	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->SetGenerateOverlapEvents(false);
	MeshComponent->SetCanEverAffectNavigation(false);
	MeshComponent->SetCastShadow(false);
}

void ATTDBattleBackgroundActor::ApplyMeshColor(UStaticMeshComponent* MeshComponent, const FLinearColor& Color) const
{
	if (!MeshComponent || !BaseMaterial)
	{
		return;
	}

	UMaterialInstanceDynamic* DynamicMaterial = MeshComponent->CreateDynamicMaterialInstance(0, BaseMaterial);
	if (DynamicMaterial)
	{
		DynamicMaterial->SetVectorParameterValue(TEXT("Color"), Color);
		DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), Color);
	}
}
