#include "Enemy/GobulinEnemyPresentationComponent.h"

#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "PaperFlipbook.h"

UGobulinEnemyPresentationComponent::UGobulinEnemyPresentationComponent()
{
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetGenerateOverlapEvents(false);
	SetCastShadow(false);
	bCanEverAffectNavigation = false;
}

void UGobulinEnemyPresentationComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!GetWorld() || !GetWorld()->IsGameWorld())
	{
		return;
	}

	FVector ViewerLocation = FVector::ZeroVector;
	const bool bHasViewer = TryGetViewerLocation(ViewerLocation);
	if (bHasViewer)
	{
		UpdateFacing(ViewerLocation);
	}
	UpdateMovementPresentation(ViewerLocation, bHasViewer);
}

void UGobulinEnemyPresentationComponent::ApplyDefinition(
	const FGobulinEnemyPaperPresentationDefinition& InDefinition)
{
	Definition = InDefinition;
	SetRelativeTransform(Definition.VisualTransformFromGround);
	SetSpriteColor(Definition.SpriteColor);
	SetTranslucentSortPriority(Definition.TranslucencySortPriority);
	SetCastShadow(Definition.bCastShadow);
	SetMaterial(0, Definition.MaterialOverride.Get());
	Stop();
	SetFlipbook(nullptr);
	SetVisibility(false, true);
	CurrentVisualState = EGobulinEnemyVisualState::Inactive;
	CurrentMoveDirection = EGobulinEnemyMoveDirection::TowardViewer;
	CurrentLocomotionAnimation = EGobulinEnemyLocomotionAnimation::Idle;
}

void UGobulinEnemyPresentationComponent::ApplyEnemyState(EEnemyState EnemyState)
{
	ApplyVisualState(GetEnemyVisualState(EnemyState));
}

void UGobulinEnemyPresentationComponent::ApplyVisualState(
	EGobulinEnemyVisualState VisualState,
	bool bRestart)
{
	if (VisualState == EGobulinEnemyVisualState::Inactive)
	{
		Stop();
		SetVisibility(false, true);
		CurrentVisualState = VisualState;
		return;
	}

	const EGobulinEnemyLocomotionAnimation DesiredLocomotion =
		VisualState == EGobulinEnemyVisualState::Alive
			&& CurrentVisualState == EGobulinEnemyVisualState::Alive
		? CurrentLocomotionAnimation
		: EGobulinEnemyLocomotionAnimation::Idle;
	UPaperFlipbook* Flipbook = VisualState == EGobulinEnemyVisualState::Death
		? Definition.GetLoadedDeathFlipbook()
		: Definition.GetLoadedLocomotionFlipbook(
			CurrentMoveDirection,
			DesiredLocomotion);
	if (!Flipbook)
	{
		Stop();
		SetFlipbook(nullptr);
		SetVisibility(false, true);
		CurrentVisualState = VisualState;
		return;
	}

	const bool bVisualStateChanged = VisualState != CurrentVisualState;
	const bool bFlipbookChanged = GetFlipbook() != Flipbook;
	if (!bVisualStateChanged && !bFlipbookChanged && !bRestart)
	{
		return;
	}

	SetVisibility(true, true);
	CurrentVisualState = VisualState;
	if (VisualState == EGobulinEnemyVisualState::Death)
	{
		SetFlipbook(Flipbook);
		SetLooping(false);
		PlayFromStart();
		return;
	}

	ApplyLocomotion(
		CurrentMoveDirection,
		DesiredLocomotion,
		bRestart,
		false);
}

void UGobulinEnemyPresentationComponent::ApplyLocomotion(
	EGobulinEnemyMoveDirection Direction,
	EGobulinEnemyLocomotionAnimation Animation,
	bool bRestart,
	bool bPreservePlaybackPhase)
{
	UPaperFlipbook* Flipbook = Definition.GetLoadedLocomotionFlipbook(Direction, Animation);
	if (!Flipbook)
	{
		Stop();
		SetFlipbook(nullptr);
		SetVisibility(false, true);
		CurrentMoveDirection = Direction;
		return;
	}

	const bool bDirectionChanged = Direction != CurrentMoveDirection;
	const bool bAnimationChanged = Animation != CurrentLocomotionAnimation;
	const bool bFlipbookChanged = GetFlipbook() != Flipbook;
	if (!bDirectionChanged && !bAnimationChanged && !bFlipbookChanged && !bRestart)
	{
		return;
	}

	const bool bCanPreservePlaybackPhase = bPreservePlaybackPhase && !bAnimationChanged;
	float NormalizedPlaybackPosition = 0.0f;
	if (bCanPreservePlaybackPhase && GetFlipbook())
	{
		const float PreviousLength = GetFlipbookLength();
		if (PreviousLength > KINDA_SMALL_NUMBER)
		{
			NormalizedPlaybackPosition = FMath::Fmod(
				FMath::Max(0.0f, GetPlaybackPosition()),
				PreviousLength) / PreviousLength;
		}
	}

	SetVisibility(true, true);
	SetFlipbook(Flipbook);
	SetLooping(true);
	if (bRestart || !bCanPreservePlaybackPhase)
	{
		SetPlaybackPosition(0.0f, false);
	}
	else
	{
		SetPlaybackPosition(NormalizedPlaybackPosition * GetFlipbookLength(), false);
	}

	Play();
	CurrentMoveDirection = Direction;
	CurrentLocomotionAnimation = Animation;
}

bool UGobulinEnemyPresentationComponent::TryGetViewerLocation(FVector& OutViewerLocation) const
{
	if (!GetWorld())
	{
		return false;
	}

	const APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (!PlayerController)
	{
		return false;
	}

	if (PlayerController->PlayerCameraManager)
	{
		OutViewerLocation = PlayerController->PlayerCameraManager->GetCameraLocation();
		return true;
	}

	if (const AActor* ViewTarget = PlayerController->GetViewTarget())
	{
		OutViewerLocation = ViewTarget->GetActorLocation();
		return true;
	}

	return false;
}

void UGobulinEnemyPresentationComponent::UpdateFacing(const FVector& ViewerLocation)
{
	if (!Definition.bFaceLocalPlayer)
	{
		return;
	}

	const FVector ToViewer = (ViewerLocation - GetComponentLocation()).GetSafeNormal2D();
	if (ToViewer.IsNearlyZero())
	{
		return;
	}

	// Paper2D 默认纸面为局部 XZ，正面法线是局部 -Y。
	const FQuat BillboardRotation = FRotationMatrix::MakeFromYZ(-ToViewer, FVector::UpVector).ToQuat();
	SetWorldRotation(BillboardRotation * Definition.VisualTransformFromGround.GetRotation());
}

void UGobulinEnemyPresentationComponent::UpdateMovementPresentation(
	const FVector& ViewerLocation,
	bool bHasViewer)
{
	if (CurrentVisualState != EGobulinEnemyVisualState::Alive)
	{
		return;
	}

	const AActor* Owner = GetOwner();
	const FVector HorizontalVelocity = Owner
		? FVector(Owner->GetVelocity().X, Owner->GetVelocity().Y, 0.0f)
		: FVector::ZeroVector;
	const float MinimumSpeed = FMath::Max(0.0f, Definition.MinimumDirectionalSpeed);
	if (HorizontalVelocity.IsNearlyZero()
		|| HorizontalVelocity.SizeSquared() < FMath::Square(MinimumSpeed))
	{
		ApplyLocomotion(
			CurrentMoveDirection,
			EGobulinEnemyLocomotionAnimation::Idle,
			false,
			false);
		return;
	}

	EGobulinEnemyMoveDirection DesiredDirection = CurrentMoveDirection;
	if (bHasViewer)
	{
		const FVector ToViewer = (ViewerLocation - GetComponentLocation()).GetSafeNormal2D();
		if (!ToViewer.IsNearlyZero())
		{
			DesiredDirection = SelectMoveDirection(HorizontalVelocity.GetSafeNormal2D(), ToViewer);
		}
	}

	ApplyLocomotion(
		DesiredDirection,
		EGobulinEnemyLocomotionAnimation::Run,
		false,
		true);
}

EGobulinEnemyMoveDirection UGobulinEnemyPresentationComponent::SelectMoveDirection(
	const FVector& MoveDirection,
	const FVector& ToViewer) const
{
	const FVector ViewerRight = FVector::CrossProduct(ToViewer, FVector::UpVector).GetSafeNormal();
	const float TowardScore = FVector::DotProduct(MoveDirection, ToViewer);
	const float RightScore = FVector::DotProduct(MoveDirection, ViewerRight);

	EGobulinEnemyMoveDirection Candidate = EGobulinEnemyMoveDirection::TowardViewer;
	float CandidateScore = TowardScore;
	const EGobulinEnemyMoveDirection Directions[] = {
		EGobulinEnemyMoveDirection::AwayFromViewer,
		EGobulinEnemyMoveDirection::ViewerLeft,
		EGobulinEnemyMoveDirection::ViewerRight
	};
	for (const EGobulinEnemyMoveDirection Direction : Directions)
	{
		const float Score = GetDirectionScore(Direction, TowardScore, RightScore);
		if (Score > CandidateScore)
		{
			Candidate = Direction;
			CandidateScore = Score;
		}
	}

	if (Candidate == CurrentMoveDirection)
	{
		return Candidate;
	}

	const float CurrentScore = GetDirectionScore(CurrentMoveDirection, TowardScore, RightScore);
	return CandidateScore >= CurrentScore + Definition.DirectionSwitchHysteresis
		? Candidate
		: CurrentMoveDirection;
}

float UGobulinEnemyPresentationComponent::GetDirectionScore(
	EGobulinEnemyMoveDirection Direction,
	float TowardScore,
	float RightScore)
{
	switch (Direction)
	{
	case EGobulinEnemyMoveDirection::AwayFromViewer:
		return -TowardScore;
	case EGobulinEnemyMoveDirection::ViewerLeft:
		return -RightScore;
	case EGobulinEnemyMoveDirection::ViewerRight:
		return RightScore;
	case EGobulinEnemyMoveDirection::TowardViewer:
	default:
		return TowardScore;
	}
}
