#include "Enemy/GobulinEnemyAIController.h"

#include "AITypes.h"
#include "Enemy/GobulinEnemyActor.h"
#include "Enemy/GobulinEnemySubsystem.h"
#include "NavigationPath.h"
#include "Navigation/PathFollowingComponent.h"

namespace
{
	FAIMoveRequest BuildEnemyMoveRequest(AActor* TargetActor, const FEnemyMoveIntent& Intent)
	{
		FAIMoveRequest MoveRequest(TargetActor);
		MoveRequest.SetAcceptanceRadius(Intent.AcceptanceRadius);
		MoveRequest.SetReachTestIncludesAgentRadius(false);
		MoveRequest.SetReachTestIncludesGoalRadius(false);
		MoveRequest.SetUsePathfinding(true);
		MoveRequest.SetAllowPartialPath(false);
		MoveRequest.SetProjectGoalLocation(true);
		MoveRequest.SetCanStrafe(false);
		return MoveRequest;
	}
}

AGobulinEnemyAIController::AGobulinEnemyAIController()
{
	bAttachToPawn = true;
}

bool AGobulinEnemyAIController::HasCompletePathToTarget(
	AActor* TargetActor,
	const FEnemyMoveIntent& Intent) const
{
	if (!IsValid(TargetActor) || !Intent.IsValid() || !GetPawn())
	{
		return false;
	}

	const FAIMoveRequest MoveRequest = BuildEnemyMoveRequest(TargetActor, Intent);
	const UPathFollowingComponent* PathFollowing = GetPathFollowingComponent();
	if (PathFollowing && PathFollowing->HasReached(MoveRequest))
	{
		// 回退阶段仍未通过接触协议时，不提交会立即完成并清空速度的 AlreadyAtGoal 请求。
		return false;
	}

	FPathFindingQuery Query;
	if (!BuildPathfindingQuery(MoveRequest, GetNavAgentLocation(), Query))
	{
		return false;
	}

	FNavPathSharedPtr Path;
	FindPathForMoveRequest(MoveRequest, Query, Path);
	return Path.IsValid() && Path->IsValid() && !Path->IsPartial();
}

EEnemyMoveStatus AGobulinEnemyAIController::RequestMoveToTarget(
	AActor* TargetActor,
	const FEnemyMoveIntent& Intent)
{
	if (!IsValid(TargetActor) || !Intent.IsValid() || !GetPawn())
	{
		return EEnemyMoveStatus::Failed;
	}

	if (bHasActiveIntent)
	{
		TGuardValue<bool> SuppressGuard(bSuppressMoveCompletion, true);
		bHasActiveIntent = false;
		StopMovement();
	}

	ActiveIntentSequence = Intent.IntentSequence;
	const FAIMoveRequest MoveRequest = BuildEnemyMoveRequest(TargetActor, Intent);

	FPathFollowingRequestResult RequestResult;
	{
		TGuardValue<bool> SuppressGuard(bSuppressMoveCompletion, true);
		RequestResult = MoveTo(MoveRequest);
	}

	switch (RequestResult.Code)
	{
	case EPathFollowingRequestResult::RequestSuccessful:
		bHasActiveIntent = true;
		return EEnemyMoveStatus::Moving;

	case EPathFollowingRequestResult::AlreadyAtGoal:
		bHasActiveIntent = false;
		return EEnemyMoveStatus::Reached;

	case EPathFollowingRequestResult::Failed:
	default:
		bHasActiveIntent = false;
		return EEnemyMoveStatus::Failed;
	}
}

void AGobulinEnemyAIController::StopEnemyMove()
{
	TGuardValue<bool> SuppressGuard(bSuppressMoveCompletion, true);
	bHasActiveIntent = false;
	ActiveIntentSequence = 0;
	StopMovement();
}

void AGobulinEnemyAIController::GetEnemyNavigationDebugData(
	FString& OutPathStatus,
	bool& bOutHasActiveIntent,
	TArray<FVector>& OutPathPoints) const
{
	OutPathStatus = TEXT("Missing");
	bOutHasActiveIntent = bHasActiveIntent;
	OutPathPoints.Reset();

	const UPathFollowingComponent* PathFollowing = GetPathFollowingComponent();
	if (!PathFollowing)
	{
		return;
	}

	switch (PathFollowing->GetStatus())
	{
	case EPathFollowingStatus::Idle:
		OutPathStatus = TEXT("Idle");
		break;
	case EPathFollowingStatus::Waiting:
		OutPathStatus = TEXT("Waiting");
		break;
	case EPathFollowingStatus::Paused:
		OutPathStatus = TEXT("Paused");
		break;
	case EPathFollowingStatus::Moving:
		OutPathStatus = TEXT("Moving");
		break;
	default:
		OutPathStatus = TEXT("Unknown");
		break;
	}

	const FNavPathSharedPtr Path = PathFollowing->GetPath();
	if (!Path.IsValid())
	{
		return;
	}

	const TArray<FNavPathPoint>& Points = Path->GetPathPoints();
	OutPathPoints.Reserve(Points.Num());
	for (const FNavPathPoint& Point : Points)
	{
		OutPathPoints.Add(Point.Location);
	}
}

void AGobulinEnemyAIController::OnMoveCompleted(
	FAIRequestID RequestID,
	const FPathFollowingResult& Result)
{
	const bool bShouldNotify = bHasActiveIntent && !bSuppressMoveCompletion;
	const int32 CompletedIntentSequence = ActiveIntentSequence;
	bHasActiveIntent = false;
	ActiveIntentSequence = 0;

	Super::OnMoveCompleted(RequestID, Result);

	if (!bShouldNotify)
	{
		return;
	}

	EEnemyMoveStatus Status = EEnemyMoveStatus::Failed;
	if (Result.Code == EPathFollowingResult::Success)
	{
		Status = EEnemyMoveStatus::Reached;
	}
	else if (Result.Code == EPathFollowingResult::Blocked)
	{
		Status = EEnemyMoveStatus::Blocked;
	}

	const AGobulinEnemyActor* EnemyActor = Cast<AGobulinEnemyActor>(GetPawn());
	UGobulinEnemySubsystem* EnemySubsystem = GetWorld()
		? GetWorld()->GetSubsystem<UGobulinEnemySubsystem>()
		: nullptr;
	if (EnemyActor && EnemySubsystem)
	{
		EnemySubsystem->NotifyEnemyMoveCompleted(
			EnemyActor->GetCombatantHandle(),
			CompletedIntentSequence,
			Status);
	}
}
