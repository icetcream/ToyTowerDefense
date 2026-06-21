#include "TTDMenuPlayerController.h"

#include "Battle/TTDBattleBackgroundActor.h"
#include "Battle/TTDBattleBuildingActor.h"
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
#include "TTDResearchSubsystem.h"

namespace
{
	const FName BattleCameraTag(TEXT("TTD.BattleCamera"));
	constexpr float BattleCameraFieldOfView = 55.0f;
	constexpr float BattleCameraBoundsPadding = 350.0f;
	constexpr float BattleCameraMinDistance = 2400.0f;
	constexpr float BattleCameraMoveSpeed = 1600.0f;
	constexpr float BattleCameraDragPanSpeed = 4.0f;
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

	FVector ClampCameraLocationToBattlefield(UWorld* World, const FVector& Location)
	{
		FBox BattlefieldBounds(ForceInit);
		if (!BuildBattlefieldBounds(World, BattlefieldBounds))
		{
			return Location;
		}

		BattlefieldBounds = BattlefieldBounds.ExpandBy(BattleCameraBoundsPadding);
		FVector ClampedLocation = Location;
		ClampedLocation.X = FMath::Clamp(ClampedLocation.X, BattlefieldBounds.Min.X, BattlefieldBounds.Max.X);
		ClampedLocation.Y = FMath::Clamp(ClampedLocation.Y, BattlefieldBounds.Min.Y, BattlefieldBounds.Max.Y);
		return ClampedLocation;
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

void ATTDMenuPlayerController::PlayerTick(const float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	TickBattleCameraMovement(DeltaTime);
}

void ATTDMenuPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent)
	{
		InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &ATTDMenuPlayerController::HandleBattleClick);
		InputComponent->BindKey(EKeys::W, IE_Pressed, this, &ATTDMenuPlayerController::HandleBattleCameraForwardPressed);
		InputComponent->BindKey(EKeys::W, IE_Released, this, &ATTDMenuPlayerController::HandleBattleCameraForwardReleased);
		InputComponent->BindKey(EKeys::S, IE_Pressed, this, &ATTDMenuPlayerController::HandleBattleCameraBackwardPressed);
		InputComponent->BindKey(EKeys::S, IE_Released, this, &ATTDMenuPlayerController::HandleBattleCameraBackwardReleased);
		InputComponent->BindKey(EKeys::D, IE_Pressed, this, &ATTDMenuPlayerController::HandleBattleCameraRightPressed);
		InputComponent->BindKey(EKeys::D, IE_Released, this, &ATTDMenuPlayerController::HandleBattleCameraRightReleased);
		InputComponent->BindKey(EKeys::A, IE_Pressed, this, &ATTDMenuPlayerController::HandleBattleCameraLeftPressed);
		InputComponent->BindKey(EKeys::A, IE_Released, this, &ATTDMenuPlayerController::HandleBattleCameraLeftReleased);
		InputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &ATTDMenuPlayerController::HandleBattleCameraDragPressed);
		InputComponent->BindKey(EKeys::RightMouseButton, IE_Released, this, &ATTDMenuPlayerController::HandleBattleCameraDragReleased);
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
	BattleCameraMoveInput = FVector2D::ZeroVector;
	bDraggingBattleCamera = false;
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
	BattleCameraMoveInput = FVector2D::ZeroVector;
	bDraggingBattleCamera = false;
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
	UTTDResearchSubsystem* ResearchSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UTTDResearchSubsystem>() : nullptr;
	FText FailureReason;
	if (!ResearchSubsystem || !ResearchSubsystem->CanConsumeToyBoxes(Loadout.SelectedToyBoxes, FailureReason))
	{
		LastBattleFailureReason = FailureReason.IsEmpty()
			? FText::FromString(TEXT("玩具盒库存不可用。"))
			: FailureReason;
		return false;
	}

	if (!BattleSubsystem || !BattleSubsystem->StartBattle(LevelId, Loadout, FailureReason))
	{
		LastBattleFailureReason = FailureReason.IsEmpty()
			? FText::FromString(TEXT("战斗系统不可用。"))
			: FailureReason;
		return false;
	}

	if (!ResearchSubsystem->ConsumeToyBoxes(Loadout.SelectedToyBoxes, FailureReason))
	{
		BattleSubsystem->EndBattle(ETTDBattleState::ConfigurationError);
		BattleSubsystem->ClearEndedBattle();
		LastBattleFailureReason = FailureReason.IsEmpty()
			? FText::FromString(TEXT("扣除玩具盒库存失败。"))
			: FailureReason;
		return false;
	}

	LastBattleFailureReason = FText::GetEmpty();
	SelectedBattleBuildingId = NAME_None;
	PendingBattleLevelId = NAME_None;
	BattleCameraMoveInput = FVector2D::ZeroVector;
	bDraggingBattleCamera = false;
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

	FText FailureReason;
	if (!BattleSubsystem || !BattleSubsystem->CanBuildBuilding(BuildingId, FailureReason))
	{
		SelectedBattleBuildingId = NAME_None;
		SetBattleStatusMessage(FailureReason.IsEmpty()
			? FText::Format(FText::FromString(TEXT("无法选择建筑：{0}。")), BuildingName)
			: FailureReason);
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
	if (bPlaced)
	{
		SelectedBattleBuildingId = NAME_None;
	}
	SetBattleStatusMessage(bPlaced ? FText::FromString(TEXT("建筑已放置。")) : OutFailureReason);
	if (UTTDBattleHUDWidget* BattleHUD = Cast<UTTDBattleHUDWidget>(CurrentWidget))
	{
		BattleHUD->RefreshSelectedBuilding();
	}
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

	if (ATTDBattleBuildingActor* Building = Cast<ATTDBattleBuildingActor>(HitResult.GetActor()))
	{
		if (UTTDBattleHUDWidget* BattleHUD = Cast<UTTDBattleHUDWidget>(CurrentWidget))
		{
			BattleHUD->SelectPlacedBuilding(Building);
			SetBattleStatusMessage(FText::FromString(TEXT("已选中建筑。")));
		}
		return;
	}

	if (SelectedBattleBuildingId.IsNone())
	{
		return;
	}

	ATTDBuildSlotActor* Slot = Cast<ATTDBuildSlotActor>(HitResult.GetActor());
	FText FailureReason;
	if (Slot)
	{
		DropSelectedBattleBuildingOnSlot(Slot, FailureReason);
		return;
	}

	if (Cast<ATTDBattleBackgroundActor>(HitResult.GetActor()))
	{
		const bool bPlaced = BattleSubsystem->TryBuildAtLocation(SelectedBattleBuildingId, HitResult.ImpactPoint, FailureReason);
		if (bPlaced)
		{
			SelectedBattleBuildingId = NAME_None;
		}
		SetBattleStatusMessage(bPlaced ? FText::FromString(TEXT("建筑已放置。")) : FailureReason);
		if (UTTDBattleHUDWidget* BattleHUD = Cast<UTTDBattleHUDWidget>(CurrentWidget))
		{
			BattleHUD->RefreshSelectedBuilding();
		}
	}
}

void ATTDMenuPlayerController::HandleBattleCameraForwardPressed()
{
	BattleCameraMoveInput.Y = 1.0f;
}

void ATTDMenuPlayerController::HandleBattleCameraForwardReleased()
{
	if (BattleCameraMoveInput.Y > 0.0f)
	{
		BattleCameraMoveInput.Y = 0.0f;
	}
}

void ATTDMenuPlayerController::HandleBattleCameraBackwardPressed()
{
	BattleCameraMoveInput.Y = -1.0f;
}

void ATTDMenuPlayerController::HandleBattleCameraBackwardReleased()
{
	if (BattleCameraMoveInput.Y < 0.0f)
	{
		BattleCameraMoveInput.Y = 0.0f;
	}
}

void ATTDMenuPlayerController::HandleBattleCameraRightPressed()
{
	BattleCameraMoveInput.X = 1.0f;
}

void ATTDMenuPlayerController::HandleBattleCameraRightReleased()
{
	if (BattleCameraMoveInput.X > 0.0f)
	{
		BattleCameraMoveInput.X = 0.0f;
	}
}

void ATTDMenuPlayerController::HandleBattleCameraLeftPressed()
{
	BattleCameraMoveInput.X = -1.0f;
}

void ATTDMenuPlayerController::HandleBattleCameraLeftReleased()
{
	if (BattleCameraMoveInput.X < 0.0f)
	{
		BattleCameraMoveInput.X = 0.0f;
	}
}

void ATTDMenuPlayerController::HandleBattleCameraDragPressed()
{
	UTTDBattleWorldSubsystem* BattleSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UTTDBattleWorldSubsystem>() : nullptr;
	if (!BattleSubsystem || !BattleSubsystem->IsBattleRunning())
	{
		return;
	}

	float MouseX = 0.0f;
	float MouseY = 0.0f;
	if (GetMousePosition(MouseX, MouseY))
	{
		LastBattleCameraDragMousePosition = FVector2D(MouseX, MouseY);
		bDraggingBattleCamera = true;
	}
}

void ATTDMenuPlayerController::HandleBattleCameraDragReleased()
{
	bDraggingBattleCamera = false;
}

void ATTDMenuPlayerController::TickBattleCameraMovement(const float DeltaTime)
{
	if (DeltaTime <= 0.0f)
	{
		return;
	}

	UTTDBattleWorldSubsystem* BattleSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UTTDBattleWorldSubsystem>() : nullptr;
	ACameraActor* CameraActor = BattleCameraActor.Get();
	if (!BattleSubsystem || !BattleSubsystem->IsBattleRunning() || !CameraActor)
	{
		bDraggingBattleCamera = false;
		BattleCameraMoveInput = FVector2D::ZeroVector;
		return;
	}

	FVector PanDelta = FVector::ZeroVector;
	if (!BattleCameraMoveInput.IsNearlyZero())
	{
		const FRotator CameraRotation = CameraActor->GetActorRotation();
		FVector Forward = FRotationMatrix(CameraRotation).GetScaledAxis(EAxis::X);
		Forward.Z = 0.0f;
		Forward.Normalize();

		FVector Right = FRotationMatrix(CameraRotation).GetScaledAxis(EAxis::Y);
		Right.Z = 0.0f;
		Right.Normalize();

		PanDelta += (Forward * BattleCameraMoveInput.Y + Right * BattleCameraMoveInput.X).GetClampedToMaxSize(1.0f)
			* BattleCameraMoveSpeed
			* DeltaTime;
	}

	if (bDraggingBattleCamera)
	{
		float MouseX = 0.0f;
		float MouseY = 0.0f;
		if (GetMousePosition(MouseX, MouseY))
		{
			const FVector2D CurrentMousePosition(MouseX, MouseY);
			const FVector2D MouseDelta = CurrentMousePosition - LastBattleCameraDragMousePosition;
			LastBattleCameraDragMousePosition = CurrentMousePosition;

			const FRotator CameraRotation = CameraActor->GetActorRotation();
			FVector Forward = FRotationMatrix(CameraRotation).GetScaledAxis(EAxis::X);
			Forward.Z = 0.0f;
			Forward.Normalize();

			FVector Right = FRotationMatrix(CameraRotation).GetScaledAxis(EAxis::Y);
			Right.Z = 0.0f;
			Right.Normalize();

			PanDelta += (-Right * MouseDelta.X + Forward * MouseDelta.Y) * BattleCameraDragPanSpeed;
		}
		else
		{
			bDraggingBattleCamera = false;
		}
	}

	if (!PanDelta.IsNearlyZero())
	{
		const FVector NewLocation = ClampCameraLocationToBattlefield(GetWorld(), CameraActor->GetActorLocation() + PanDelta);
		CameraActor->SetActorLocation(NewLocation);
	}
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
