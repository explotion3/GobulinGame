#pragma once

#include "CoreMinimal.h"
#include "Combat/CombatantHandle.h"

/** 非权威、非序列化的敌人调试快照；只用于 PIE 可视化、Gameplay Debugger 与一次性日志。 */
struct GOBULINGAME_API FGobulinEnemyDebugSnapshot
{
	FCombatantHandle Handle;
	FString ActorName;
	FString EnemyState;
	FString MoveStatus;
	FString PathStatus;
	FString MovementMode;
	FString LastMovementEvent;

	FVector ActorLocation = FVector::ZeroVector;
	FVector TargetLocation = FVector::ZeroVector;
	FVector IntentDestination = FVector::ZeroVector;
	FVector ActualVelocity = FVector::ZeroVector;
	FVector DriveVelocityChange = FVector::ZeroVector;
	FVector SeparationVelocityChange = FVector::ZeroVector;
	FVector LiftVelocityChange = FVector::ZeroVector;
	FVector GroundSupportLocation = FVector::ZeroVector;
	TArray<FVector> PathPoints;

	int32 LocalNeighborCount = 0;
	float LocalPressure = 0.0f;
	float CombatCapsuleRadius = 0.0f;
	float CombatCapsuleHalfHeight = 0.0f;
	float CrowdRadius = 0.0f;
	float NeighborRange = 0.0f;
	float CrowdNavigationRetryRemaining = 0.0f;
	float StateAge = 0.0f;
	float MoveStatusAge = 0.0f;
	float DistanceMovedLastSample = 0.0f;
	float TargetDistanceDeltaLastSample = 0.0f;
	float NoTargetProgressTime = 0.0f;
	float TargetToIntentDistance = 0.0f;
	float NavigationRecoveryHeightDelta = 0.0f;
	float NavigationRecoveryTargetMovement = 0.0f;
	float NavigationRecoveryTargetProgress = 0.0f;
	float GroundSupportRadius = 0.0f;
	float GroundSnapDownHeight = 0.0f;

	bool bHasTarget = false;
	bool bInContact = false;
	bool bGrounded = false;
	bool bPathIntentActive = false;
	bool bCanWalkOffLedges = false;
	bool bHasGroundSupportSample = false;
	bool bHasCenterGroundSupport = false;
	bool bTargetProgressStalled = false;
	bool bNavigationPathRejected = false;
};
