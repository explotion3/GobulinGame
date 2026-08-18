#include "Enemy/GobulinEnemyPresentationTypes.h"

#include "PaperFlipbook.h"

bool FGobulinEnemyBodyDefinition::IsValid() const
{
	return FMath::IsFinite(CapsuleRadius)
		&& CapsuleRadius >= 1.0f
		&& FMath::IsFinite(CapsuleHalfHeight)
		&& CapsuleHalfHeight >= CapsuleRadius;
}

TSoftObjectPtr<UPaperFlipbook> FGobulinEnemyDirectionalFlipbookSet::GetForDirection(
	EGobulinEnemyMoveDirection Direction) const
{
	switch (Direction)
	{
	case EGobulinEnemyMoveDirection::AwayFromViewer:
		return AwayFromViewer;
	case EGobulinEnemyMoveDirection::ViewerLeft:
		return ViewerLeft;
	case EGobulinEnemyMoveDirection::ViewerRight:
		return ViewerRight;
	case EGobulinEnemyMoveDirection::TowardViewer:
	default:
		return TowardViewer;
	}
}

bool FGobulinEnemyDirectionalFlipbookSet::SetForDirection(
	EGobulinEnemyMoveDirection Direction,
	UPaperFlipbook* Flipbook)
{
	switch (Direction)
	{
	case EGobulinEnemyMoveDirection::AwayFromViewer:
		AwayFromViewer = Flipbook;
		return true;
	case EGobulinEnemyMoveDirection::ViewerLeft:
		ViewerLeft = Flipbook;
		return true;
	case EGobulinEnemyMoveDirection::ViewerRight:
		ViewerRight = Flipbook;
		return true;
	case EGobulinEnemyMoveDirection::TowardViewer:
	default:
		TowardViewer = Flipbook;
		return true;
	}
}

bool FGobulinEnemyDirectionalFlipbookSet::IsValid() const
{
	return !TowardViewer.IsNull()
		&& !AwayFromViewer.IsNull()
		&& !ViewerLeft.IsNull()
		&& !ViewerRight.IsNull();
}

TSoftObjectPtr<UPaperFlipbook> FGobulinEnemyFlipbookSet::GetForLocomotion(
	EGobulinEnemyMoveDirection Direction,
	EGobulinEnemyLocomotionAnimation Animation) const
{
	return Animation == EGobulinEnemyLocomotionAnimation::Run
		? Run.GetForDirection(Direction)
		: Idle.GetForDirection(Direction);
}

bool FGobulinEnemyFlipbookSet::SetForLocomotion(
	EGobulinEnemyMoveDirection Direction,
	EGobulinEnemyLocomotionAnimation Animation,
	UPaperFlipbook* Flipbook)
{
	return Animation == EGobulinEnemyLocomotionAnimation::Run
		? Run.SetForDirection(Direction, Flipbook)
		: Idle.SetForDirection(Direction, Flipbook);
}

bool FGobulinEnemyFlipbookSet::IsValid() const
{
	return Idle.IsValid()
		&& Run.IsValid()
		&& !Death.IsNull();
}

bool FGobulinEnemyPaperPresentationDefinition::IsValid() const
{
	const FVector Location = VisualTransformFromGround.GetLocation();
	const FVector Scale = VisualTransformFromGround.GetScale3D();
	return Flipbooks.IsValid()
		&& !Location.ContainsNaN()
		&& !Scale.ContainsNaN()
		&& FMath::Abs(Scale.X) > KINDA_SMALL_NUMBER
		&& FMath::Abs(Scale.Y) > KINDA_SMALL_NUMBER
		&& FMath::Abs(Scale.Z) > KINDA_SMALL_NUMBER
		&& VisualTransformFromGround.IsValid()
		&& FMath::IsFinite(MinimumDirectionalSpeed)
		&& MinimumDirectionalSpeed >= 0.0f
		&& FMath::IsFinite(DirectionSwitchHysteresis)
		&& DirectionSwitchHysteresis >= 0.0f
		&& DirectionSwitchHysteresis <= 1.0f;
}

UPaperFlipbook* FGobulinEnemyPaperPresentationDefinition::GetLoadedLocomotionFlipbook(
	EGobulinEnemyMoveDirection Direction,
	EGobulinEnemyLocomotionAnimation Animation) const
{
	UPaperFlipbook* Flipbook = Flipbooks.GetForLocomotion(Direction, Animation).Get();
	if (!Flipbook && Direction != EGobulinEnemyMoveDirection::TowardViewer)
	{
		Flipbook = Flipbooks.GetForLocomotion(
			EGobulinEnemyMoveDirection::TowardViewer,
			Animation).Get();
	}
	return Flipbook;
}

UPaperFlipbook* FGobulinEnemyPaperPresentationDefinition::GetLoadedDeathFlipbook() const
{
	return Flipbooks.Death.Get();
}

EGobulinEnemyVisualState GetEnemyVisualState(EEnemyState EnemyState)
{
	switch (EnemyState)
	{
	case EEnemyState::Dying:
		return EGobulinEnemyVisualState::Death;
	case EEnemyState::Inactive:
		return EGobulinEnemyVisualState::Inactive;
	default:
		return EGobulinEnemyVisualState::Alive;
	}
}
