#include "Combat/FPSWeaponAnimator.h"

#include "Combat/FPSWeaponOverlayComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"

UFPSWeaponAnimator::UFPSWeaponAnimator()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UFPSWeaponAnimator::SetOverlay(UFPSWeaponOverlayComponent* InOverlay)
{
	Overlay = InOverlay;
	if (!Overlay)
	{
		return;
	}

	ActiveClip = IdleClip;
	bLockedByAction = false;
	Overlay->PlayClip(ActiveClip);

	if (const APawn* Pawn = Cast<APawn>(GetOwner()))
	{
		PreviousControlRotation = Pawn->GetControlRotation();
	}
}

void UFPSWeaponAnimator::PlayClip(FName ClipId, bool bRestart)
{
	if (!Overlay)
	{
		return;
	}

	Overlay->PlayClip(ClipId, bRestart);
	ActiveClip = ClipId;
	bLockedByAction = (ClipId != IdleClip && ClipId != MoveClip);
}

void UFPSWeaponAnimator::TriggerFire()
{
	if (!Overlay)
	{
		return;
	}

	Overlay->PlayClip(FireClip, true);
	Overlay->AddRecoil();
	ActiveClip = FireClip;
	bLockedByAction = true;
}

void UFPSWeaponAnimator::TriggerReload()
{
	PlayClip(ReloadClip);
}

void UFPSWeaponAnimator::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!Overlay)
	{
		return;
	}

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	const float MovementSpeed = Character ? Character->GetVelocity().Size2D() : 0.0f;
	const bool bIsGrounded = Character && Character->GetCharacterMovement() ? Character->GetCharacterMovement()->IsMovingOnGround() : true;

	FVector2D LookDelta = FVector2D::ZeroVector;
	if (APawn* Pawn = Character ? Cast<APawn>(Character) : Cast<APawn>(GetOwner()))
	{
		const FRotator CurrentControlRotation = Pawn->GetControlRotation();
		const FRotator Delta = (CurrentControlRotation - PreviousControlRotation).GetNormalized();
		LookDelta = FVector2D(Delta.Yaw, Delta.Pitch);
		PreviousControlRotation = CurrentControlRotation;
	}

	Overlay->UpdateProceduralMotion(DeltaTime, MovementSpeed, LookDelta, bIsGrounded);

	if (!bLockedByAction)
	{
		const FName DesiredClip = MovementSpeed > MoveThreshold ? MoveClip : IdleClip;
		if (DesiredClip != ActiveClip)
		{
			Overlay->PlayClip(DesiredClip);
			ActiveClip = DesiredClip;
		}
	}
}
