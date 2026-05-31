#include "Battle/TTDBuildSlotActor.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

ATTDBuildSlotActor::ATTDBuildSlotActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SlotBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("SlotBounds"));
	SlotBounds->SetBoxExtent(FVector(80.0f, 80.0f, 20.0f));
	SlotBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SlotBounds->SetCollisionResponseToAllChannels(ECR_Ignore);
	SlotBounds->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	RootComponent = SlotBounds;

	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(SlotBounds);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VisualMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 2.5f));
	VisualMesh->SetRelativeScale3D(FVector(1.6f, 1.6f, 0.05f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SlotMeshAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (SlotMeshAsset.Succeeded())
	{
		VisualMesh->SetStaticMesh(SlotMeshAsset.Object);
	}
}

void ATTDBuildSlotActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyVisualMaterial();
}

void ATTDBuildSlotActor::ConfigureSlot(const FName InSlotId, const FIntPoint InGridCoordinate, const int32 InHeightLevel, const FName InHeightEffectId)
{
	SlotId = InSlotId;
	GridCoordinate = InGridCoordinate;
	HeightLevel = InHeightLevel;
	HeightEffectId = InHeightEffectId;
	ApplyVisualMaterial();
}

void ATTDBuildSlotActor::SetOccupyingBuilding(AActor* Building)
{
	OccupyingBuilding = Building;
}

void ATTDBuildSlotActor::ClearOccupyingBuilding(AActor* Building)
{
	if (!Building || OccupyingBuilding.Get() == Building)
	{
		OccupyingBuilding.Reset();
	}
}

void ATTDBuildSlotActor::ApplyVisualMaterial()
{
	if (VisualMesh && VisualMaterial)
	{
		VisualMesh->SetMaterial(0, VisualMaterial);
	}
}
