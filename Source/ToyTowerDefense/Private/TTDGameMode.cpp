#include "TTDGameMode.h"

#include "TTDMenuPlayerController.h"

ATTDGameMode::ATTDGameMode()
{
	PlayerControllerClass = ATTDMenuPlayerController::StaticClass();
	DefaultPawnClass = nullptr;
}
