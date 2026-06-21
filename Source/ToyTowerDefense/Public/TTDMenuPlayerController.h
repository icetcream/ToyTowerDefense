#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TTDMenuPlayerController.generated.h"

class ATTDBuildSlotActor;
class ACameraActor;
class UUserWidget;
struct FTTDBattleLoadout;

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
	void ShowWarehouse();

	UFUNCTION(BlueprintCallable, Category = "Toy Tower Defense|Menu")
	void ShowBattleLevelSelect();

	UFUNCTION(BlueprintCallable, Category = "Toy Tower Defense|Menu")
	void ShowBattlePreparation(FName LevelId);

	UFUNCTION(BlueprintCallable, Category = "Toy Tower Defense|Menu")
	bool ShowBattleLevel();

	bool StartPreparedBattle(FName LevelId, const FTTDBattleLoadout& Loadout);

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
	TSubclassOf<UUserWidget> WarehouseWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Toy Tower Defense|Menu")
	TSubclassOf<UUserWidget> BattleLevelSelectWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Toy Tower Defense|Menu")
	TSubclassOf<UUserWidget> BattlePreparationWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Toy Tower Defense|Menu")
	TSubclassOf<UUserWidget> BattleHUDWidgetClass;

private:
	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> CurrentWidget;

	FName SelectedBattleBuildingId;
	FName PendingBattleLevelId;
	FText LastBattleFailureReason;
	TWeakObjectPtr<ACameraActor> BattleCameraActor;

	void ShowWidget(TSubclassOf<UUserWidget> WidgetClass);
	void HandleBattleClick();
	void ApplyBattleCameraView();
	ACameraActor* FindPlacedBattleCamera() const;
	ACameraActor* SpawnFallbackBattleCamera();
};
