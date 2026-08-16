#include "Player/GobulinPlayerController.h"

#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GobulinGameCameraManager.h"
#include "InputMappingContext.h"

AGobulinPlayerController::AGobulinPlayerController()
{
	PlayerCameraManagerClass = AGobulinGameCameraManager::StaticClass();
}

void AGobulinPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!IsLocalPlayerController())
	{
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (!Subsystem)
	{
		return;
	}

	for (UInputMappingContext* MappingContext : DefaultMappingContexts)
	{
		if (MappingContext)
		{
			Subsystem->AddMappingContext(MappingContext, 0);
		}
	}
}
