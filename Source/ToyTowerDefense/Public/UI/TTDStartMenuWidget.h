#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TTDStartMenuWidget.generated.h"

UCLASS()
class TOYTOWERDEFENSE_API UTTDStartMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	float EntranceElapsed = 0.0f;

	void HandleStartClicked();
};
