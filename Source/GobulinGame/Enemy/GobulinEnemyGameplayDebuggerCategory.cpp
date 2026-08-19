#include "Enemy/GobulinEnemyGameplayDebuggerCategory.h"

#if WITH_GAMEPLAY_DEBUGGER

#include "AIController.h"
#include "Enemy/GobulinEnemyActor.h"
#include "Enemy/GobulinEnemySubsystem.h"
#include "Engine/World.h"
#include "GameplayDebuggerTypes.h"

FGobulinEnemyGameplayDebuggerCategory::FGobulinEnemyGameplayDebuggerCategory()
{
	bShowOnlyWithDebugActor = true;
	CollectDataInterval = 0.1f;
}

TSharedRef<FGameplayDebuggerCategory> FGobulinEnemyGameplayDebuggerCategory::MakeInstance()
{
	return MakeShareable(new FGobulinEnemyGameplayDebuggerCategory());
}

void FGobulinEnemyGameplayDebuggerCategory::CollectData(
	APlayerController* OwnerPC,
	AActor* DebugActor)
{
	(void)OwnerPC;

	const AGobulinEnemyActor* EnemyActor = Cast<AGobulinEnemyActor>(DebugActor);
	if (!EnemyActor)
	{
		if (const AAIController* Controller = Cast<AAIController>(DebugActor))
		{
			EnemyActor = Cast<AGobulinEnemyActor>(Controller->GetPawn());
		}
	}
	if (!EnemyActor)
	{
		AddTextLine(TEXT("{yellow}请将 Gameplay Debugger 目标对准一个 GobulinEnemyActor。"));
		return;
	}

	const UGobulinEnemySubsystem* EnemySubsystem = EnemyActor->GetWorld()
		? EnemyActor->GetWorld()->GetSubsystem<UGobulinEnemySubsystem>()
		: nullptr;
	FGobulinEnemyDebugSnapshot Snapshot;
	if (!EnemySubsystem
		|| !EnemySubsystem->GetEnemyDebugSnapshot(EnemyActor->GetCombatantHandle(), Snapshot))
	{
		AddTextLine(TEXT("{red}该 Actor 没有有效的敌人运行时记录。"));
		return;
	}

	AddTextLine(FString::Printf(
		TEXT("{yellow}Handle:{white} %s  {yellow}Actor:{white} %s"),
		*Snapshot.Handle.ToString(),
		*Snapshot.ActorName));
	AddTextLine(FString::Printf(
		TEXT("{yellow}State:{white} %s (%.2fs)  {yellow}Move:{white} %s (%.2fs)"),
		*Snapshot.EnemyState,
		Snapshot.StateAge,
		*Snapshot.MoveStatus,
		Snapshot.MoveStatusAge));
	AddTextLine(FString::Printf(
		TEXT("{yellow}Path:{white} %s  ActiveIntent:%d  Points:%d  Target:%d  Contact:%d"),
		*Snapshot.PathStatus,
		Snapshot.bPathIntentActive ? 1 : 0,
		Snapshot.PathPoints.Num(),
		Snapshot.bHasTarget ? 1 : 0,
		Snapshot.bInContact ? 1 : 0));
	AddTextLine(FString::Printf(
		TEXT("{yellow}Movement:{white} %s  Grounded:%d  CanWalkOffLedges:%d  Velocity:(%.1f, %.1f, %.1f)"),
		*Snapshot.MovementMode,
		Snapshot.bGrounded ? 1 : 0,
		Snapshot.bCanWalkOffLedges ? 1 : 0,
		Snapshot.ActualVelocity.X,
		Snapshot.ActualVelocity.Y,
		Snapshot.ActualVelocity.Z));
	AddTextLine(FString::Printf(
		TEXT("{yellow}GroundSupport:{white} %s  Radius:%.1fcm  SnapDown:%.1fcm  Location:%s"),
		Snapshot.bHasGroundSupportSample
			? (Snapshot.bHasCenterGroundSupport ? TEXT("Valid") : TEXT("Invalid"))
			: TEXT("Unknown"),
		Snapshot.GroundSupportRadius,
		Snapshot.GroundSnapDownHeight,
		*Snapshot.GroundSupportLocation.ToCompactString()));
	AddTextLine(FString::Printf(
		TEXT("{yellow}Crowd:{white} Neighbors:%d  Pressure:%.2f  Radius:%.1f  Range:%.1f  Repath:%.2fs"),
		Snapshot.LocalNeighborCount,
		Snapshot.LocalPressure,
		Snapshot.CrowdRadius,
		Snapshot.NeighborRange,
		Snapshot.CrowdNavigationRetryRemaining));
	AddTextLine(FString::Printf(
		TEXT("{yellow}Progress:{white} Move=%.1fcm/0.5s  GoalDelta=%.1fcm/0.5s  NoProgress=%.2fs  Stalled:%d"),
		Snapshot.DistanceMovedLastSample,
		Snapshot.TargetDistanceDeltaLastSample,
		Snapshot.NoTargetProgressTime,
		Snapshot.bTargetProgressStalled ? 1 : 0));
	AddTextLine(FString::Printf(
		TEXT("{yellow}NavRejected:{white} %d  Recovery Z/Target/Goal: %.1f / %.1f / %.1fcm"),
		Snapshot.bNavigationPathRejected ? 1 : 0,
		Snapshot.NavigationRecoveryHeightDelta,
		Snapshot.NavigationRecoveryTargetMovement,
		Snapshot.NavigationRecoveryTargetProgress));
	AddTextLine(FString::Printf(
		TEXT("{yellow}Target-Intent Delta:{white} %.1fcm  {yellow}LastEvent:{white} %s"),
		Snapshot.TargetToIntentDistance,
		*Snapshot.LastMovementEvent));
	AddTextLine(FString::Printf(
		TEXT("{green}Drive:{white} %s  {orange}Separation:{white} %s  {magenta}Lift:{white} %s"),
		*Snapshot.DriveVelocityChange.ToCompactString(),
		*Snapshot.SeparationVelocityChange.ToCompactString(),
		*Snapshot.LiftVelocityChange.ToCompactString()));

	const FColor BodyColor = Snapshot.bTargetProgressStalled ? FColor::Red : FColor::Cyan;
	AddShape(FGameplayDebuggerShape::MakeCapsule(
		Snapshot.ActorLocation,
		Snapshot.CombatCapsuleRadius,
		Snapshot.CombatCapsuleHalfHeight,
		BodyColor,
		TEXT("Combat capsule")));
	AddShape(FGameplayDebuggerShape::MakeCircle(
		Snapshot.ActorLocation,
		FVector::UpVector,
		Snapshot.CrowdRadius,
		FColor::Yellow,
		TEXT("Crowd body")));
	AddShape(FGameplayDebuggerShape::MakeCircle(
		Snapshot.ActorLocation,
		FVector::UpVector,
		Snapshot.NeighborRange,
		FColor(128, 128, 128),
		TEXT("Neighbor query")));
	if (Snapshot.bHasGroundSupportSample)
	{
		AddShape(FGameplayDebuggerShape::MakeCircle(
			Snapshot.GroundSupportLocation,
			FVector::UpVector,
			Snapshot.GroundSupportRadius,
			Snapshot.bHasCenterGroundSupport ? FColor::Green : FColor::Red,
			TEXT("Center ground support")));
	}
	AddShape(FGameplayDebuggerShape::MakeArrow(
		Snapshot.ActorLocation,
		Snapshot.ActorLocation + Snapshot.ActualVelocity * 0.25f,
		12.0f,
		2.0f,
		FColor::Cyan,
		TEXT("Velocity")));
	if (Snapshot.bHasTarget)
	{
		AddShape(FGameplayDebuggerShape::MakeSegment(
			Snapshot.ActorLocation,
			Snapshot.TargetLocation,
			FColor::Green,
			TEXT("Target")));
		AddShape(FGameplayDebuggerShape::MakePoint(
			Snapshot.IntentDestination,
			12.0f,
			FColor::White,
			TEXT("Move intent")));
	}
	if (!Snapshot.DriveVelocityChange.IsNearlyZero())
	{
		AddShape(FGameplayDebuggerShape::MakeArrow(
			Snapshot.ActorLocation,
			Snapshot.ActorLocation + Snapshot.DriveVelocityChange * 8.0f,
			12.0f,
			2.0f,
			FColor::Green,
			TEXT("Crowd drive")));
	}
	if (!Snapshot.SeparationVelocityChange.IsNearlyZero())
	{
		AddShape(FGameplayDebuggerShape::MakeArrow(
			Snapshot.ActorLocation,
			Snapshot.ActorLocation + Snapshot.SeparationVelocityChange * 8.0f,
			12.0f,
			2.0f,
			FColor::Orange,
			TEXT("Crowd separation")));
	}
	if (!Snapshot.LiftVelocityChange.IsNearlyZero())
	{
		AddShape(FGameplayDebuggerShape::MakeArrow(
			Snapshot.ActorLocation,
			Snapshot.ActorLocation + Snapshot.LiftVelocityChange * 8.0f,
			12.0f,
			2.0f,
			FColor::Magenta,
			TEXT("Crowd lift")));
	}
	if (Snapshot.PathPoints.Num() > 1)
	{
		AddShape(FGameplayDebuggerShape::MakePolyline(
			Snapshot.PathPoints,
			2.0f,
			FColor::White,
			TEXT("Navigation path")));
	}
}

#endif
