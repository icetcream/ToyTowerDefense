#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TTDMainMenuWidget.generated.h"

class UTextBlock;

UCLASS()
class TOYTOWERDEFENSE_API UTTDMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatusText;

	float EntranceElapsed = 0.0f;

	void HandleWarehouseClicked();
	void HandleLevelChallengeClicked();
	void HandleResearchLabClicked();
};
