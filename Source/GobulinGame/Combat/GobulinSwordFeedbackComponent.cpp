#include "Combat/GobulinSwordFeedbackComponent.h"

#include "Camera/CameraShakeBase.h"
#include "Camera/PlayerCameraManager.h"
#include "Data/GobulinSwordDefinition.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Combat/GobulinHitStopSubsystem.h"

UGobulinSwordFeedbackComponent::UGobulinSwordFeedbackComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UGobulinSwordFeedbackComponent::BeginPlay()
{
	Super::BeginPlay();
	BindToSwordCombat();
}

void UGobulinSwordFeedbackComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindFromSwordCombat();
	Super::EndPlay(EndPlayReason);
}

void UGobulinSwordFeedbackComponent::SetSwordCombat(UGobulinSwordCombatComponent* InSwordCombat)
{
	if (SwordCombat == InSwordCombat)
	{
		return;
	}

	UnbindFromSwordCombat();
	SwordCombat = InSwordCombat;

	if (HasBegunPlay())
	{
		BindToSwordCombat();
	}
}

void UGobulinSwordFeedbackComponent::HandleAttackStateChanged(
	EGobulinSwordAttackState /*PreviousState*/,
	EGobulinSwordAttackState NewState)
{
	if (NewState == EGobulinSwordAttackState::Attacking)
	{
		bHitCameraShakePlayed = false;
		bHitStopPlayed = false;
	}
}

void UGobulinSwordFeedbackComponent::HandleSwingTriggered()
{
	if (!IsLocalPlayerFeedback() || !SwordCombat)
	{
		return;
	}

	const UGobulinSwordDefinition* SwordDefinition = SwordCombat->GetSwordDefinition();
	if (SwordDefinition && SwordDefinition->SwingSound)
	{
		UGameplayStatics::PlaySound2D(this, SwordDefinition->SwingSound);
	}
}

void UGobulinSwordFeedbackComponent::HandleHitConfirmed(
	AActor* /*HitActor*/,
	FVector HitLocation,
	FVector /*HitNormal*/,
	float /*AppliedDamage*/,
	bool /*bKilled*/)
{
	if (!IsLocalPlayerFeedback() || !SwordCombat)
	{
		return;
	}

	const UGobulinSwordDefinition* SwordDefinition = SwordCombat->GetSwordDefinition();
	if (!SwordDefinition)
	{
		return;
	}

	if (SwordDefinition->HitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, SwordDefinition->HitSound, HitLocation);
	}

	if (!bHitStopPlayed && SwordDefinition->HitStopDuration > 0.0f)
	{
		if (UWorld* World = GetWorld())
		{
			if (UGobulinHitStopSubsystem* HitStop = World->GetSubsystem<UGobulinHitStopSubsystem>())
			{
				HitStop->RequestHitStop(
					SwordDefinition->HitStopDuration,
					SwordDefinition->HitStopTimeDilation);
				bHitStopPlayed = true;
			}
		}
	}

	if (bHitCameraShakePlayed || !SwordDefinition->HitCameraShake)
	{
		return;
	}

	const APawn* Pawn = Cast<APawn>(GetOwner());
	APlayerController* PlayerController = Pawn
		? Cast<APlayerController>(Pawn->GetController())
		: nullptr;
	if (!PlayerController || !PlayerController->IsLocalController() || !PlayerController->PlayerCameraManager)
	{
		return;
	}

	PlayerController->PlayerCameraManager->StartCameraShake(
		SwordDefinition->HitCameraShake,
		SwordDefinition->HitCameraShakeScale);
	bHitCameraShakePlayed = true;
}

void UGobulinSwordFeedbackComponent::BindToSwordCombat()
{
	if (!SwordCombat)
	{
		return;
	}

	SwordCombat->OnAttackStateChanged.AddDynamic(this, &UGobulinSwordFeedbackComponent::HandleAttackStateChanged);
	SwordCombat->OnSwingTriggered.AddDynamic(this, &UGobulinSwordFeedbackComponent::HandleSwingTriggered);
	SwordCombat->OnHitConfirmed.AddDynamic(this, &UGobulinSwordFeedbackComponent::HandleHitConfirmed);
}

void UGobulinSwordFeedbackComponent::UnbindFromSwordCombat()
{
	if (!SwordCombat)
	{
		return;
	}

	SwordCombat->OnAttackStateChanged.RemoveDynamic(this, &UGobulinSwordFeedbackComponent::HandleAttackStateChanged);
	SwordCombat->OnSwingTriggered.RemoveDynamic(this, &UGobulinSwordFeedbackComponent::HandleSwingTriggered);
	SwordCombat->OnHitConfirmed.RemoveDynamic(this, &UGobulinSwordFeedbackComponent::HandleHitConfirmed);
}

bool UGobulinSwordFeedbackComponent::IsLocalPlayerFeedback() const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	return Pawn && Pawn->IsLocallyControlled();
}
