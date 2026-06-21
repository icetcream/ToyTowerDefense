#include "TTDMenuPlayerController.h"

#include "Battle/TTDBattleBackgroundActor.h"
#include "Battle/TTDBattleCastleActor.h"
#include "Battle/TTDBattleSpawnPointActor.h"
#include "Battle/TTDBuildSlotActor.h"
#include "Blueprint/UserWidget.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"
#include "TTDBattleWorldSubsystem.h"
#include "UI/TTDBattleLevelSelectWidget.h"
#include "UI/TTDBattlePreparationWidget.h"
#include "UI/TTDBattleHUDWidget.h"
#include "UI/TTDMainMenuWidget.h"
#include "UI/TTDResearchLabWidget.h"
#include "UI/TTDStartMenuWidget.h"
#include "UI/TTDUIDisplayNames.h"
#include "UI/TTDWarehouseWidget.h"

namespace
{
	const FName BattleCameraTag(TEXT("TTD.BattleCamera"));
	constexpr float BattleCameraFieldOfView = 55.0f;
	constexpr float BattleCameraBoundsPadding = 350.0f;
	constexpr float BattleCameraMinDistance = 2400.0f;
	const FVector BattleCameraDirection(-1.0f, -1.0f, 1.25f);

	void AddActorBounds(const AActor* Actor, FBox& Bounds)
	{
		if (!IsValid(Actor))
		{
			return;
		}

		FBox ActorBounds = Actor->GetComponentsBoundingBox(true, true);
		if (!ActorBounds.IsValid)
		{
			const FVector ActorLocation = Actor->GetActorLocation();
			ActorBounds = FBox(ActorLocation - FVector(100.0f, 100.0f, 50.0f), ActorLocation + FVector(100.0f, 100.0f, 100.0f));
		}

		Bounds += ActorBounds;
	}

	template<typename ActorType>
	void AddActorTypeBounds(UWorld* World, FBox& Bounds)
	{
		if (!World)
		{
			return;
		}

		for (TActorIterator<ActorType> It(World); It; ++It)
		{
			AddActorBounds(*It, Bounds);
		}
	}

	bool BuildBattlefieldBounds(UWorld* World, FBox& OutBounds)
	{
		OutBounds = FBox(ForceInit);
		AddActorTypeBounds<ATTDBattleBackgroundActor>(World, OutBounds);
		AddActorTypeBounds<ATTDBattleCastleActor>(World, OutBounds);
		AddActorTypeBounds<ATTDBattleSpawnPointActor>(World, OutBounds);
		AddActorTypeBounds<ATTDBuildSlotActor>(World, OutBounds);
		return OutBounds.IsValid != 0;
	}

	void AlignCameraToBattlefield(UWorld* World, ACameraActor* CameraActor)
	{
		if (!World || !CameraActor)
		{
			return;
		}

		FBox BattlefieldBounds(ForceInit);
		if (!BuildBattlefieldBounds(World, BattlefieldBounds))
		{
			return;
		}

		BattlefieldBounds = BattlefieldBounds.ExpandBy(BattleCameraBoundsPadding);
		FVector LookAtLocation = BattlefieldBounds.GetCenter();
		LookAtLocation.Z = FMath::Max(LookAtLocation.Z, BattlefieldBounds.Min.Z + 80.0f);

		const FVector BoundsExtent = BattlefieldBounds.GetExtent();
		const float HalfDiagonal2D = FVector2D(BoundsExtent.X, BoundsExtent.Y).Size();
		const float HalfFovRadians = FMath::DegreesToRadians(BattleCameraFieldOfView * 0.5f);
		const float DistanceForBounds = HalfFovRadians > KINDA_SMALL_NUMBER
			? HalfDiagonal2D / FMath::Tan(HalfFovRadians)
			: BattleCameraMinDistance;
		const float CameraDistance = FMath::Max(BattleCameraMinDistance, DistanceForBounds);
		const FVector CameraLocation = LookAtLocation + BattleCameraDirection.GetSafeNormal() * CameraDistance;

		FRotator CameraRotation = (LookAtLocation - CameraLocation).Rotation();
		CameraRotation.Roll = 0.0f;

		CameraActor->SetActorLocation(CameraLocation);
		CameraActor->SetActorRotation(CameraRotation);
		CameraActor->Tags.AddUnique(BattleCameraTag);

		if (UCameraComponent* CameraComponent = CameraActor->GetCameraComponent())
		{
			CameraComponent->SetRelativeLocation(FVector::ZeroVector);
			CameraComponent->SetRelativeRotation(FRotator::ZeroRotator);
			CameraComponent->SetFieldOfView(BattleCameraFieldOfView);
		}
	}
}

ATTDMenuPlayerController::ATTDMenuPlayerController()
{
	bShowMouseCursor = true;
	StartMenuWidgetClass = UTTDStartMenuWidget::StaticClass();
	MainMenuWidgetClass = UTTDMainMenuWidget::StaticClass();
	ResearchLabWidgetClass = UTTDResearchLabWidget::StaticClass();
	WarehouseWidgetClass = UTTDWarehouseWidget::StaticClass();
	BattleLevelSelectWidgetClass = UTTDBattleLevelSelectWidget::StaticClass();
	BattlePreparationWidgetClass = UTTDBattlePreparationWidget::StaticClass();
	BattleHUDWidgetClass = UTTDBattleHUDWidget::StaticClass();
}

void ATTDMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();
	ShowStartScreen();
}

void ATTDMenuPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent)
	{
		InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &ATTDMenuPlayerController::HandleBattleClick);
	}
}

void ATTDMenuPlayerController::ShowStartScreen()
{
	ShowWidget(StartMenuWidgetClass);
}

void ATTDMenuPlayerController::ShowMainScreen()
{
	ShowWidget(MainMenuWidgetClass);
}

void ATTDMenuPlayerController::ShowResearchLab()
{
	ShowWidget(ResearchLabWidgetClass);
}

void ATTDMenuPlayerController::ShowWarehouse()
{
	ShowWidget(WarehouseWidgetClass);
}

void ATTDMenuPlayerController::ShowBattleLevelSelect()
{
	if (UTTDBattleWorldSubsystem* BattleSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UTTDBattleWorldSubsystem>() : nullptr)
	{
		if (BattleSubsystem->GetBattleState() != ETTDBattleState::Running)
		{
			BattleSubsystem->ClearEndedBattle();
		}
	}

	SelectedBattleBuildingId = NAME_None;
	ShowWidget(BattleLevelSelectWidgetClass);
}

void ATTDMenuPlayerController::ShowBattlePreparation(const FName LevelId)
{
	PendingBattleLevelId = LevelId;
	ShowWidget(BattlePreparationWidgetClass);

	if (UTTDBattlePreparationWidget* PreparationWidget = Cast<UTTDBattlePreparationWidget>(CurrentWidget))
	{
		PreparationWidget->SetLevelId(LevelId);
	}
}

bool ATTDMenuPlayerController::ShowBattleLevel()
{
	UTTDBattleWorldSubsystem* BattleSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UTTDBattleWorldSubsystem>() : nullptr;
	FText FailureReason;
	if (!BattleSubsystem || !BattleSubsystem->StartDefaultBattle(FailureReason))
	{
		LastBattleFailureReason = FailureReason.IsEmpty()
			? FText::FromString(TEXT("战斗系统不可用。"))
			: FailureReason;
		return false;
	}

	LastBattleFailureReason = FText::GetEmpty();
	SelectedBattleBuildingId = NAME_None;
	ApplyBattleCameraView();
	ShowWidget(BattleHUDWidgetClass);

	if (CurrentWidget)
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(CurrentWidget->TakeWidget());
		InputMode.SetHideCursorDuringCapture(false);
		SetInputMode(InputMode);
	}

	bShowMouseCursor = true;
	bEnableClickEvents = true;
	SetIgnoreMoveInput(true);
	SetIgnoreLookInput(true);
	return true;
}

bool ATTDMenuPlayerController::StartPreparedBattle(const FName LevelId, const FTTDBattleLoadout& Loadout)
{
	UTTDBattleWorldSubsystem* BattleSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UTTDBattleWorldSubsystem>() : nullptr;
	FText FailureReason;
	if (!BattleSubsystem || !BattleSubsystem->StartBattle(LevelId, Loadout, FailureReason))
	{
		LastBattleFailureReason = FailureReason.IsEmpty()
			? FText::FromString(TEXT("战斗系统不可用。"))
			: FailureReason;
		return false;
	}

	LastBattleFailureReason = FText::GetEmpty();
	SelectedBattleBuildingId = NAME_None;
	PendingBattleLevelId = NAME_None;
	ApplyBattleCameraView();
	ShowWidget(BattleHUDWidgetClass);

	if (CurrentWidget)
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(CurrentWidget->TakeWidget());
		InputMode.SetHideCursorDuringCapture(false);
		SetInputMode(InputMode);
	}

	bShowMouseCursor = true;
	bEnableClickEvents = true;
	SetIgnoreMoveInput(true);
	SetIgnoreLookInput(true);
	return true;
}

void ATTDMenuPlayerController::SelectBattleBuilding(const FName BuildingId)
{
	const UTTDBattleWorldSubsystem* BattleSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UTTDBattleWorldSubsystem>() : nullptr;
	const FTTDBuildingDefinition* BuildingDefinition = BattleSubsystem ? BattleSubsystem->FindBuildingDefinition(BuildingId) : nullptr;
	const FText BuildingName = BuildingDefinition
		? TTDUIDisplayNames::DisplayNameOrFallback(BuildingDefinition->DisplayName, BuildingId)
		: TTDUIDisplayNames::KnownId(BuildingId);

	if (!BattleSubsystem || !BattleSubsystem->IsBuildingAvailable(BuildingId))
	{
		SelectedBattleBuildingId = NAME_None;
		SetBattleStatusMessage(FText::Format(FText::FromString(TEXT("未携带 {0} 所需图纸。")), BuildingName));
		if (UTTDBattleHUDWidget* BattleHUD = Cast<UTTDBattleHUDWidget>(CurrentWidget))
		{
			BattleHUD->RefreshSelectedBuilding();
		}
		return;
	}

	SelectedBattleBuildingId = BuildingId;
	SetBattleStatusMessage(FText::Format(FText::FromString(TEXT("已选择建筑：{0}")), BuildingName));
	if (UTTDBattleHUDWidget* BattleHUD = Cast<UTTDBattleHUDWidget>(CurrentWidget))
	{
		BattleHUD->RefreshSelectedBuilding();
	}
}

void ATTDMenuPlayerController::BeginDragBattleBuilding(const FName BuildingId)
{
	SelectBattleBuilding(BuildingId);
}

bool ATTDMenuPlayerController::DropSelectedBattleBuildingOnSlot(ATTDBuildSlotActor* Slot, FText& OutFailureReason)
{
	if (SelectedBattleBuildingId.IsNone())
	{
		OutFailureReason = FText::FromString(TEXT("尚未选择建筑。"));
		return false;
	}

	UTTDBattleWorldSubsystem* BattleSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UTTDBattleWorldSubsystem>() : nullptr;
	if (!BattleSubsystem)
	{
		OutFailureReason = FText::FromString(TEXT("战斗系统不可用。"));
		return false;
	}

	const bool bPlaced = BattleSubsystem->TryBuildOnSlot(SelectedBattleBuildingId, Slot, OutFailureReason);
	SetBattleStatusMessage(bPlaced ? FText::FromString(TEXT("建筑已放置。")) : OutFailureReason);
	return bPlaced;
}

void ATTDMenuPlayerController::SetBattleStatusMessage(const FText& Message)
{
	if (UTTDBattleHUDWidget* BattleHUD = Cast<UTTDBattleHUDWidget>(CurrentWidget))
	{
		BattleHUD->SetStatusMessage(Message);
	}
}

void ATTDMenuPlayerController::ShowWidget(const TSubclassOf<UUserWidget> WidgetClass)
{
	if (CurrentWidget)
	{
		CurrentWidget->RemoveFromParent();
		CurrentWidget = nullptr;
	}

	if (!WidgetClass)
	{
		return;
	}

	CurrentWidget = CreateWidget<UUserWidget>(this, WidgetClass);
	if (!CurrentWidget)
	{
		return;
	}

	CurrentWidget->AddToViewport();

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(CurrentWidget->TakeWidget());
	SetInputMode(InputMode);
	bShowMouseCursor = true;
}

void ATTDMenuPlayerController::HandleBattleClick()
{
	if (SelectedBattleBuildingId.IsNone())
	{
		return;
	}

	UTTDBattleWorldSubsystem* BattleSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UTTDBattleWorldSubsystem>() : nullptr;
	if (!BattleSubsystem || !BattleSubsystem->IsBattleRunning())
	{
		return;
	}

	FHitResult HitResult;
	if (!GetHitResultUnderCursor(ECC_Visibility, true, HitResult))
	{
		return;
	}

	ATTDBuildSlotActor* Slot = Cast<ATTDBuildSlotActor>(HitResult.GetActor());
	if (!Slot)
	{
		return;
	}

	FText FailureReason;
	DropSelectedBattleBuildingOnSlot(Slot, FailureReason);
}

void ATTDMenuPlayerController::ApplyBattleCameraView()
{
	ACameraActor* CameraActor = FindPlacedBattleCamera();
	if (!CameraActor)
	{
		CameraActor = SpawnFallbackBattleCamera();
	}

	if (!CameraActor)
	{
		return;
	}

	AlignCameraToBattlefield(GetWorld(), CameraActor);

	BattleCameraActor = CameraActor;
	FViewTargetTransitionParams TransitionParams;
	TransitionParams.BlendTime = 0.0f;
	SetViewTarget(CameraActor, TransitionParams);
}

ACameraActor* ATTDMenuPlayerController::FindPlacedBattleCamera() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	for (TActorIterator<ACameraActor> It(World); It; ++It)
	{
		ACameraActor* CameraActor = *It;
		if (IsValid(CameraActor) && CameraActor->ActorHasTag(BattleCameraTag))
		{
			return CameraActor;
		}
	}

	return nullptr;
}

ACameraActor* ATTDMenuPlayerController::SpawnFallbackBattleCamera()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	const FVector CameraLocation(-1600.0f, -1600.0f, 2300.0f);
	const FVector LookAtLocation(0.0f, 0.0f, 0.0f);
	FRotator CameraRotation = (LookAtLocation - CameraLocation).Rotation();
	CameraRotation.Roll = 0.0f;

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ACameraActor* CameraActor = World->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), CameraLocation, CameraRotation, SpawnParameters);
	if (CameraActor)
	{
		CameraActor->Tags.AddUnique(BattleCameraTag);
		if (UCameraComponent* CameraComponent = CameraActor->GetCameraComponent())
		{
			CameraComponent->SetFieldOfView(BattleCameraFieldOfView);
		}
	}

	return CameraActor;
}
