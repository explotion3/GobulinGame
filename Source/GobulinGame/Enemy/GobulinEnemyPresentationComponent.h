#pragma once

#include "CoreMinimal.h"
#include "Enemy/EnemyReactionTypes.h"
#include "Enemy/GobulinEnemyPresentationTypes.h"
#include "PaperFlipbookComponent.h"
#include "GobulinEnemyPresentationComponent.generated.h"

/** 当前 Actor 后端的 Paper2D 表现组件；不承担碰撞、伤害或玩法状态推进。 */
UCLASS(ClassGroup = (Enemy), meta = (BlueprintSpawnableComponent))
class GOBULINGAME_API UGobulinEnemyPresentationComponent : public UPaperFlipbookComponent
{
	GENERATED_BODY()

public:
	UGobulinEnemyPresentationComponent();

	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	void ApplyDefinition(const FGobulinEnemyPaperPresentationDefinition& InDefinition);
	void ApplyEnemyState(EEnemyState EnemyState);
	void SetLocomotionSuspended(bool bSuspended);
	void BeginHitFlash(bool bLethal, const FGobulinEnemyReactionDefinition& ReactionDefinition);
	void BeginDeathEffects(const FGobulinEnemyReactionDefinition& ReactionDefinition);
	void NotifyDeathLanded();
	bool IsDeathPresentationComplete() const { return bDeathPresentationComplete; }
	void ApplyVisualState(EGobulinEnemyVisualState VisualState, bool bRestart = false);
	void ApplyLocomotion(
		EGobulinEnemyMoveDirection Direction,
		EGobulinEnemyLocomotionAnimation Animation,
		bool bRestart = false,
		bool bPreservePlaybackPhase = true);

	const FGobulinEnemyPaperPresentationDefinition& GetDefinition() const { return Definition; }
	EGobulinEnemyVisualState GetCurrentVisualState() const { return CurrentVisualState; }
	EGobulinEnemyMoveDirection GetCurrentMoveDirection() const { return CurrentMoveDirection; }
	EGobulinEnemyLocomotionAnimation GetCurrentLocomotionAnimation() const { return CurrentLocomotionAnimation; }

private:
	UPROPERTY(Transient)
	FGobulinEnemyPaperPresentationDefinition Definition;

	UPROPERTY(Transient)
	EGobulinEnemyVisualState CurrentVisualState = EGobulinEnemyVisualState::Inactive;

	UPROPERTY(Transient)
	EGobulinEnemyMoveDirection CurrentMoveDirection = EGobulinEnemyMoveDirection::TowardViewer;

	UPROPERTY(Transient)
	EGobulinEnemyLocomotionAnimation CurrentLocomotionAnimation = EGobulinEnemyLocomotionAnimation::Idle;

	bool bLocomotionSuspended = false;
	bool bDeathEffectsActive = false;
	bool bDeathPresentationComplete = false;
	float HitFlashStartTime = 0.0f;
	float HitFlashDuration = 0.0f;
	float DeathStartTime = 0.0f;
	float DeathLandedTime = 0.0f;
	FGobulinEnemyReactionDefinition ActiveReactionDefinition;

	bool TryGetViewerLocation(FVector& OutViewerLocation) const;
	void UpdateReactionEffects(float WorldTime);
	void ResetReactionMaterialData();
	void UpdateFacing(const FVector& ViewerLocation);
	void UpdateMovementPresentation(const FVector& ViewerLocation, bool bHasViewer);
	EGobulinEnemyMoveDirection SelectMoveDirection(
		const FVector& MoveDirection,
		const FVector& ToViewer) const;
	static float GetDirectionScore(
		EGobulinEnemyMoveDirection Direction,
		float TowardScore,
		float RightScore);
};
