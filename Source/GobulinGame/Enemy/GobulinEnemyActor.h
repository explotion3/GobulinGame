#pragma once

#include "CoreMinimal.h"
#include "Combat/CombatantEndpoint.h"
#include "Enemy/EnemyNavigationTypes.h"
#include "Enemy/EnemyReactionTypes.h"
#include "Enemy/EnemyStateTypes.h"
#include "GameFramework/Character.h"
#include "UObject/PrimaryAssetId.h"
#include "GobulinEnemyActor.generated.h"

class UCapsuleComponent;
struct FGobulinEnemyCrowdDefinition;
class UGobulinEnemyArchetype;
class UGobulinEnemyMovementComponent;
class UGobulinEnemyPresentationComponent;
class USceneComponent;

/** Thin Actor representation for one enemy. Gameplay state is owned by UGobulinEnemySubsystem. */
UCLASS(Blueprintable, NotPlaceable)
class GOBULINGAME_API AGobulinEnemyActor : public ACharacter, public ICombatantEndpoint
{
	GENERATED_BODY()

public:
	AGobulinEnemyActor(const FObjectInitializer& ObjectInitializer);

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Landed(const FHitResult& Hit) override;
	virtual FCombatDamageResult ResolveCombatDamage_Implementation(const FCombatDamageRequest& Request) override;

	UFUNCTION(BlueprintPure, Category = "Enemy")
	FCombatantHandle GetCombatantHandle() const { return CombatantHandle; }

	UFUNCTION(BlueprintPure, Category = "Enemy")
	FPrimaryAssetId GetArchetypeId() const { return ArchetypeId; }

	UFUNCTION(BlueprintPure, Category = "Enemy|Components")
	UCapsuleComponent* GetCollisionCapsule() const { return GetCapsuleComponent(); }

	UFUNCTION(BlueprintPure, Category = "Enemy|Components")
	USceneComponent* GetFeetAnchor() const { return FeetAnchor; }

	UFUNCTION(BlueprintPure, Category = "Enemy|Components")
	UGobulinEnemyPresentationComponent* GetPresentationComponent() const { return PresentationComponent; }

	UFUNCTION(BlueprintPure, Category = "Enemy|Components")
	UGobulinEnemyMovementComponent* GetEnemyMovementComponent() const;

	void InitializeEnemy(FCombatantHandle InHandle, const UGobulinEnemyArchetype& Archetype);
	void ApplyEnemyState(EEnemyState NewState);
	bool HasCompleteNavigationPathToTarget(AActor* TargetActor, const FEnemyMoveIntent& Intent) const;
	EEnemyMoveStatus RequestMoveToTarget(AActor* TargetActor, const FEnemyMoveIntent& Intent);
	void StopEnemyMovement();
	/** Actor 后端将水平分离转换为移动输入、将垂直抬升转换为冲量；不直接改 Actor Transform。 */
	FVector ApplyCrowdVelocityChange(
		const FVector& VelocityChange,
		float MaximumLiftSpeed,
		float DeltaTime);
	FVector ApplyCrowdFallbackDrive(
		const FVector& WorldDirection,
		float DesiredSpeed,
		const FGobulinEnemyCrowdDefinition& CrowdDefinition,
		float DeltaTime);
	void ApplyEnemyImpact(const FVector& LaunchVelocity, bool bLethal);
	void BeginDeathPhysics(const FVector& LaunchVelocity);
	bool IsEnemyGrounded() const;
	bool IsDeathPresentationComplete() const;
	void ReleaseEnemyHandle();

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> FeetAnchor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UGobulinEnemyPresentationComponent> PresentationComponent;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Enemy", meta = (AllowPrivateAccess = "true"))
	FCombatantHandle CombatantHandle;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Enemy", meta = (AllowPrivateAccess = "true"))
	FPrimaryAssetId ArchetypeId;

	UPROPERTY(Transient)
	FGobulinEnemyReactionDefinition ReactionDefinition;
};
