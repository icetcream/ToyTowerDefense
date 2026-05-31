#include "TTDMenuPlayerController.h"

#include "Battle/TTDBuildSlotActor.h"
#include "Blueprint/UserWidget.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"
#include "TTDBattleWorldSubsystem.h"
#include "UI/TTDBattleHUDWidget.h"
#include "UI/TTDMainMenuWidget.h"
#include "UI/TTDResearchLabWidget.h"
#include "UI/TTDStartMenuWidget.h"

ATTDMenuPlayerController::ATTDMenuPlayerController()
{
	bShowMouseCursor = true;
	StartMenuWidgetClass = UTTDStartMenuWidget::StaticClass();
	MainMenuWidgetClass = UTTDMainMenuWidget::StaticClass();
	ResearchLabWidgetClass = UTTDResearchLabWidget::StaticClass();
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

bool ATTDMenuPlayerController::ShowBattleLevel()
{
	UTTDBattleWorldSubsystem* BattleSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UTTDBattleWorldSubsystem>() : nullptr;
	FText FailureReason;
	if (!BattleSubsystem || !BattleSubsystem->StartDefaultBattle(FailureReason))
	{
		LastBattleFailureReason = FailureReason.IsEmpty()
			? FText::FromString(TEXT("Battle subsystem is unavailable."))
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

void ATTDMenuPlayerController::SelectBattleBuilding(const FName BuildingId)
{
	SelectedBattleBuildingId = BuildingId;
	SetBattleStatusMessage(FText::Format(FText::FromString(TEXT("Selected building: {0}")), FText::FromName(BuildingId)));
}

void ATTDMenuPlayerController::BeginDragBattleBuilding(const FName BuildingId)
{
	SelectBattleBuilding(BuildingId);
}

bool ATTDMenuPlayerController::DropSelectedBattleBuildingOnSlot(ATTDBuildSlotActor* Slot, FText& OutFailureReason)
{
	if (SelectedBattleBuildingId.IsNone())
	{
		OutFailureReason = FText::FromString(TEXT("No building selected."));
		return false;
	}

	UTTDBattleWorldSubsystem* BattleSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UTTDBattleWorldSubsystem>() : nullptr;
	if (!BattleSubsystem)
	{
		OutFailureReason = FText::FromString(TEXT("Battle subsystem is unavailable."));
		return false;
	}

	const bool bPlaced = BattleSubsystem->TryBuildOnSlot(SelectedBattleBuildingId, Slot, OutFailureReason);
	SetBattleStatusMessage(bPlaced ? FText::FromString(TEXT("Building placed.")) : OutFailureReason);
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

	static const FName BattleCameraTag(TEXT("TTD.BattleCamera"));
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
		CameraActor->Tags.AddUnique(TEXT("TTD.BattleCamera"));
		if (UCameraComponent* CameraComponent = CameraActor->GetCameraComponent())
		{
			CameraComponent->SetFieldOfView(55.0f);
		}
	}

	return CameraActor;
}
