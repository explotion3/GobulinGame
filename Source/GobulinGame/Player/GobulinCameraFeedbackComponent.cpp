#include "Player/GobulinCameraFeedbackComponent.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"

DEFINE_LOG_CATEGORY_STATIC(LogGobulinCameraFeedback, Log, All);

UGobulinCameraFeedbackComponent::UGobulinCameraFeedbackComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UGobulinCameraFeedbackComponent::BeginPlay()
{
	Super::BeginPlay();

	SetComponentTickEnabled(IsLocalPlayerFeedback());
}

void UGobulinCameraFeedbackComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearCameraFeedback();
	Super::EndPlay(EndPlayReason);
}

void UGobulinCameraFeedbackComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!Camera || !IsLocalPlayerFeedback())
	{
		return;
	}

	const APawn* Pawn = Cast<APawn>(GetOwner());
	const UCharacterMovementComponent* CharacterMovement = Pawn
		? Pawn->FindComponentByClass<UCharacterMovementComponent>()
		: nullptr;

	const FVector Velocity = Pawn ? Pawn->GetVelocity() : FVector::ZeroVector;
	const FVector PlanarVelocity(Velocity.X, Velocity.Y, 0.0f);
	const float PlanarSpeed = PlanarVelocity.Size();
	const bool bIsGrounded = CharacterMovement && CharacterMovement->IsMovingOnGround();
	const float MaximumMovementSpeed = CharacterMovement
		? FMath::Max(CharacterMovement->MaxWalkSpeed, MovementBobSpeedThreshold)
		: FMath::Max(500.0f, MovementBobSpeedThreshold);
	const float MovementSpeedRange = FMath::Max(
		MaximumMovementSpeed - MovementBobSpeedThreshold,
		KINDA_SMALL_NUMBER);
	const float MovementAlpha = FMath::Clamp(
		(PlanarSpeed - MovementBobSpeedThreshold) / MovementSpeedRange,
		0.0f,
		1.0f);

	const bool bShouldBob = bEnableMovementBob && bIsGrounded && MovementAlpha > 0.0f;
	const float TargetBobWeight = bShouldBob ? MovementAlpha : 0.0f;
	MovementBobWeight = FMath::FInterpTo(MovementBobWeight, TargetBobWeight, DeltaTime, MovementBobBlendSpeed);

	if (bShouldBob && MovementBobFrequency > 0.0f)
	{
		const float PhaseSpeedMultiplier = FMath::Max(MovementAlpha, 0.25f);
		MovementBobPhase = FMath::Fmod(
			MovementBobPhase + DeltaTime * MovementBobFrequency * UE_TWO_PI * PhaseSpeedMultiplier,
			UE_TWO_PI);
	}

	const FVector RightDirection = Pawn ? Pawn->GetActorRightVector() : FVector::YAxisVector;
	const float StrafeSpeed = FVector::DotProduct(PlanarVelocity, RightDirection);
	const float StrafeAlpha = FMath::Clamp(StrafeSpeed / MaximumMovementSpeed, -1.0f, 1.0f);
	const float StrafeRollDirection = bInvertStrafeRoll ? 1.0f : -1.0f;
	const float TargetStrafeRoll = bEnableStrafeRoll && bIsGrounded
		? StrafeRollDirection * StrafeAlpha * StrafeRollAngle
		: 0.0f;
	CurrentStrafeRoll = FMath::FInterpTo(CurrentStrafeRoll, TargetStrafeRoll, DeltaTime, StrafeRollInterpSpeed);

	const float VerticalOffset = FMath::Sin(MovementBobPhase) * MovementBobAmplitude * MovementBobWeight;
	ApplyCameraFeedback(VerticalOffset, CurrentStrafeRoll);
}

void UGobulinCameraFeedbackComponent::SetCamera(UCameraComponent* InCamera)
{
	if (Camera == InCamera)
	{
		return;
	}

	ClearCameraFeedback();
	Camera = InCamera;
}

bool UGobulinCameraFeedbackComponent::IsLocalPlayerFeedback() const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	return Pawn && Pawn->IsLocallyControlled();
}

void UGobulinCameraFeedbackComponent::ApplyCameraFeedback(float VerticalOffset, float RollOffset)
{
	if (!Camera)
	{
		return;
	}

	FTransform AdditiveTransform = FTransform::Identity;
	AdditiveTransform.SetLocation(FVector(0.0f, 0.0f, VerticalOffset));
	AdditiveTransform.SetRotation(FQuat(FRotator(0.0f, 0.0f, RollOffset)));

	// bUsePawnControlRotation updates the camera's component rotation every frame.
	// AdditiveOffset is applied after that update and therefore preserves the roll.
	Camera->ClearAdditiveOffset();
	Camera->AddAdditiveOffset(AdditiveTransform, 0.0f);
}

void UGobulinCameraFeedbackComponent::ClearCameraFeedback()
{
	if (!Camera)
	{
		return;
	}

	Camera->ClearAdditiveOffset();
}
