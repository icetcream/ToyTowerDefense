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

private:
	void HandleStartClicked();
};
