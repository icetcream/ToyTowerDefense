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

private:
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatusText;

	void HandleWarehouseClicked();
	void HandleLevelChallengeClicked();
	void HandleResearchLabClicked();
};
