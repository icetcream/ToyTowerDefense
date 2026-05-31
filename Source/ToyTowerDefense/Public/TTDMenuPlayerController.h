#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TTDMenuPlayerController.generated.h"

class ATTDBuildSlotActor;
class ACameraActor;
class UUserWidget;

UCLASS()
class TOYTOWERDEFENSE_API ATTDMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ATTDMenuPlayerController();

	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	UFUNCTION(BlueprintCallable, Category = "Toy Tower Defense|Menu")
	void ShowStartScreen();

	UFUNCTION(BlueprintCallable, Category = "Toy Tower Defense|Menu")
	void ShowMainScreen();

	UFUNCTION(BlueprintCallable, Category = "Toy Tower Defense|Menu")
	void ShowResearchLab();

	UFUNCTION(BlueprintCallable, Category = "Toy Tower Defense|Menu")
	bool ShowBattleLevel();

	UFUNCTION(BlueprintPure, Category = "Toy Tower Defense|Battle")
	FText GetLastBattleFailureReason() const { return LastBattleFailureReason; }

	UFUNCTION(BlueprintCallable, Category = "Toy Tower Defense|Battle")
	void SelectBattleBuilding(FName BuildingId);

	UFUNCTION(BlueprintCallable, Category = "Toy Tower Defense|Battle")
	void BeginDragBattleBuilding(FName BuildingId);

	UFUNCTION(BlueprintCallable, Category = "Toy Tower Defense|Battle")
	bool DropSelectedBattleBuildingOnSlot(ATTDBuildSlotActor* Slot, FText& OutFailureReason);

	UFUNCTION(BlueprintPure, Category = "Toy Tower Defense|Battle")
	FName GetSelectedBattleBuildingId() const { return SelectedBattleBuildingId; }

	void SetBattleStatusMessage(const FText& Message);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Toy Tower Defense|Menu")
	TSubclassOf<UUserWidget> StartMenuWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Toy Tower Defense|Menu")
	TSubclassOf<UUserWidget> MainMenuWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Toy Tower Defense|Menu")
	TSubclassOf<UUserWidget> ResearchLabWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Toy Tower Defense|Menu")
	TSubclassOf<UUserWidget> BattleHUDWidgetClass;

private:
	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> CurrentWidget;

	FName SelectedBattleBuildingId;
	FText LastBattleFailureReason;
	TWeakObjectPtr<ACameraActor> BattleCameraActor;

	void ShowWidget(TSubclassOf<UUserWidget> WidgetClass);
	void HandleBattleClick();
	void ApplyBattleCameraView();
	ACameraActor* FindPlacedBattleCamera() const;
	ACameraActor* SpawnFallbackBattleCamera();
};
