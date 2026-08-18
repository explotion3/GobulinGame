#include "Enemy/GobulinEnemyPresentationComponent.h"

#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "PaperFlipbook.h"

namespace
{
	constexpr int32 HitFlashAmountDataIndex = 0;
	constexpr int32 LethalFlashSelectorDataIndex = 1;
	constexpr int32 ReservedReactionDataIndex2 = 2;
	constexpr int32 ReservedReactionDataIndex3 = 3;
	constexpr int32 DeathDarkenAmountDataIndex = 4;
	constexpr int32 OpacityDataIndex = 5;
}

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

	UpdateReactionEffects(GetWorld()->GetTimeSeconds());

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
	ResetReactionMaterialData();
	Stop();
	SetFlipbook(nullptr);
	SetVisibility(false, true);
	CurrentVisualState = EGobulinEnemyVisualState::Inactive;
	CurrentMoveDirection = EGobulinEnemyMoveDirection::TowardViewer;
	CurrentLocomotionAnimation = EGobulinEnemyLocomotionAnimation::Idle;
	bLocomotionSuspended = false;
	bDeathEffectsActive = false;
	bDeathPresentationComplete = false;
	HitFlashStartTime = 0.0f;
	HitFlashDuration = 0.0f;
	DeathStartTime = 0.0f;
	DeathLandedTime = 0.0f;
	ActiveReactionDefinition = FGobulinEnemyReactionDefinition();
}

void UGobulinEnemyPresentationComponent::ApplyEnemyState(EEnemyState EnemyState)
{
	ApplyVisualState(GetEnemyVisualState(EnemyState));
}

void UGobulinEnemyPresentationComponent::SetLocomotionSuspended(bool bSuspended)
{
	if (bLocomotionSuspended == bSuspended)
	{
		return;
	}

	bLocomotionSuspended = bSuspended;
	if (bLocomotionSuspended && CurrentVisualState == EGobulinEnemyVisualState::Alive)
	{
		ApplyLocomotion(
			EGobulinEnemyMoveDirection::TowardViewer,
			EGobulinEnemyLocomotionAnimation::Idle,
			false,
			false);
	}
}

void UGobulinEnemyPresentationComponent::BeginHitFlash(
	bool bLethal,
	const FGobulinEnemyReactionDefinition& ReactionDefinition)
{
	if (!GetWorld())
	{
		return;
	}

	HitFlashStartTime = GetWorld()->GetTimeSeconds();
	HitFlashDuration = bLethal
		? ReactionDefinition.LethalFlashDuration
		: ReactionDefinition.HitFlashDuration;
	SetCustomPrimitiveDataFloat(LethalFlashSelectorDataIndex, bLethal ? 1.0f : 0.0f);
	SetCustomPrimitiveDataFloat(HitFlashAmountDataIndex, HitFlashDuration > 0.0f ? 1.0f : 0.0f);
}

void UGobulinEnemyPresentationComponent::BeginDeathEffects(
	const FGobulinEnemyReactionDefinition& ReactionDefinition)
{
	if (!GetWorld())
	{
		return;
	}

	ActiveReactionDefinition = ReactionDefinition;
	bDeathEffectsActive = true;
	bDeathPresentationComplete = false;
	DeathStartTime = GetWorld()->GetTimeSeconds();
	DeathLandedTime = 0.0f;
	SetCustomPrimitiveDataFloat(DeathDarkenAmountDataIndex, 0.0f);
	SetCustomPrimitiveDataFloat(OpacityDataIndex, 1.0f);
	BeginHitFlash(true, ReactionDefinition);
}

void UGobulinEnemyPresentationComponent::NotifyDeathLanded()
{
	if (bDeathEffectsActive && DeathLandedTime <= 0.0f && GetWorld())
	{
		DeathLandedTime = GetWorld()->GetTimeSeconds();
	}
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

	UMaterialInterface* DesiredMaterial = VisualState == EGobulinEnemyVisualState::Death
		&& !Definition.DeathMaterialOverride.IsNull()
		? Definition.DeathMaterialOverride.Get()
		: Definition.MaterialOverride.Get();
	SetMaterial(0, DesiredMaterial);

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
		SetLooping(Definition.bLoopDeathFlipbook);
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
	if (CurrentVisualState != EGobulinEnemyVisualState::Alive || bLocomotionSuspended)
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

void UGobulinEnemyPresentationComponent::UpdateReactionEffects(float WorldTime)
{
	if (HitFlashDuration > 0.0f)
	{
		const float FlashAlpha = 1.0f - FMath::Clamp(
			(WorldTime - HitFlashStartTime) / HitFlashDuration,
			0.0f,
			1.0f);
		SetCustomPrimitiveDataFloat(HitFlashAmountDataIndex, FlashAlpha);
		if (FlashAlpha <= 0.0f)
		{
			HitFlashDuration = 0.0f;
		}
	}

	if (!bDeathEffectsActive)
	{
		return;
	}

	const float DarkenElapsed = WorldTime
		- DeathStartTime
		- ActiveReactionDefinition.DeathDarkenDelay;
	const float DarkenAlpha = ActiveReactionDefinition.DeathDarkenDuration > KINDA_SMALL_NUMBER
		? FMath::Clamp(DarkenElapsed / ActiveReactionDefinition.DeathDarkenDuration, 0.0f, 1.0f)
		: (DarkenElapsed >= 0.0f ? 1.0f : 0.0f);
	SetCustomPrimitiveDataFloat(
		DeathDarkenAmountDataIndex,
		FMath::SmoothStep(0.0f, 1.0f, DarkenAlpha)
			* ActiveReactionDefinition.DeathMaximumDarken);

	const float FailsafeFadeStartTime = DeathStartTime
		+ FMath::Max(
			0.0f,
			ActiveReactionDefinition.DeathMaximumDuration
				- ActiveReactionDefinition.CorpseFadeDuration);
	const float FadeStartTime = DeathLandedTime > 0.0f
		? FMath::Min(
			DeathLandedTime + ActiveReactionDefinition.CorpseSettleDelay,
			FailsafeFadeStartTime)
		: FailsafeFadeStartTime;
	const float FadeAlpha = FMath::Clamp(
		(WorldTime - FadeStartTime) / ActiveReactionDefinition.CorpseFadeDuration,
		0.0f,
		1.0f);
	SetCustomPrimitiveDataFloat(OpacityDataIndex, 1.0f - FMath::SmoothStep(0.0f, 1.0f, FadeAlpha));
	if (FadeAlpha >= 1.0f)
	{
		bDeathEffectsActive = false;
		bDeathPresentationComplete = true;
	}
}

void UGobulinEnemyPresentationComponent::ResetReactionMaterialData()
{
	SetCustomPrimitiveDataFloat(HitFlashAmountDataIndex, 0.0f);
	SetCustomPrimitiveDataFloat(LethalFlashSelectorDataIndex, 0.0f);
	SetCustomPrimitiveDataFloat(ReservedReactionDataIndex2, 0.0f);
	SetCustomPrimitiveDataFloat(ReservedReactionDataIndex3, 0.0f);
	SetCustomPrimitiveDataFloat(DeathDarkenAmountDataIndex, 0.0f);
	SetCustomPrimitiveDataFloat(OpacityDataIndex, 1.0f);
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
