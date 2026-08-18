#include "Enemy/GobulinEnemyAIController.h"

#include "AITypes.h"
#include "Enemy/GobulinEnemyActor.h"
#include "Enemy/GobulinEnemySubsystem.h"
#include "Navigation/PathFollowingComponent.h"

AGobulinEnemyAIController::AGobulinEnemyAIController()
{
	bAttachToPawn = true;
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
	FAIMoveRequest MoveRequest(TargetActor);
	MoveRequest.SetAcceptanceRadius(Intent.AcceptanceRadius);
	MoveRequest.SetReachTestIncludesAgentRadius(false);
	MoveRequest.SetReachTestIncludesGoalRadius(false);
	MoveRequest.SetUsePathfinding(true);
	MoveRequest.SetAllowPartialPath(false);
	MoveRequest.SetProjectGoalLocation(true);
	MoveRequest.SetCanStrafe(false);

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
