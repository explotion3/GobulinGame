#include "Enemy/GobulinEnemyMovementComponent.h"

#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"

void UGobulinEnemyMovementComponent::ConfigureGroundSupport(
	float InSupportRadius,
	float InSnapDownHeight)
{
	GroundSupportRadius = FMath::IsFinite(InSupportRadius)
		? FMath::Max(0.5f, InSupportRadius)
		: 3.0f;
	GroundSnapDownHeight = FMath::IsFinite(InSnapDownHeight)
		? FMath::Max(0.5f, InSnapDownHeight)
		: 8.0f;

	const UCapsuleComponent* Capsule = CharacterOwner
		? CharacterOwner->GetCapsuleComponent()
		: nullptr;
	const float CapsuleRadius = Capsule ? Capsule->GetScaledCapsuleRadius() : GroundSupportRadius;
	GroundSupportRadius = FMath::Min(GroundSupportRadius, CapsuleRadius);

	// CharacterMovement 默认允许走下边缘；基础怪潮不再启用沿边寻找替代路线的保护。
	bCanWalkOffLedges = true;
	PerchRadiusThreshold = FMath::Max(0.0f, CapsuleRadius - GroundSupportRadius);
	PerchAdditionalHeight = 0.0f;
	bHasGroundSupportSample = false;
}

bool UGobulinEnemyMovementComponent::ShouldCatchAir(
	const FFindFloorResult& OldFloor,
	const FFindFloorResult& NewFloor)
{
	if (Super::ShouldCatchAir(OldFloor, NewFloor))
	{
		return true;
	}

	if (!HasValidData() || !UpdatedComponent)
	{
		return false;
	}

	FFindFloorResult CenterFloor;
	ComputeFloorDist(
		UpdatedComponent->GetComponentLocation(),
		GroundSnapDownHeight,
		GroundSnapDownHeight,
		CenterFloor,
		GroundSupportRadius);

	bHasGroundSupportSample = true;
	bHasCenterGroundSupport = CenterFloor.IsWalkableFloor()
		&& CenterFloor.GetDistanceToFloor() <= GroundSnapDownHeight;
	GroundSupportLocation = CenterFloor.HitResult.bBlockingHit
		? CenterFloor.HitResult.ImpactPoint
		: GetActorFeetLocation() - FVector::UpVector * GroundSnapDownHeight;
	return !bHasCenterGroundSupport;
}

void UGobulinEnemyMovementComponent::OnMovementModeChanged(
	EMovementMode PreviousMovementMode,
	uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);

	if (IsFalling())
	{
		bHasGroundSupportSample = true;
		bHasCenterGroundSupport = false;
		GroundSupportLocation = GetActorFeetLocation()
			- FVector::UpVector * GroundSnapDownHeight;
	}
	else if (IsMovingOnGround() && CurrentFloor.IsWalkableFloor())
	{
		bHasGroundSupportSample = true;
		bHasCenterGroundSupport = true;
		GroundSupportLocation = CurrentFloor.HitResult.ImpactPoint;
	}
}
