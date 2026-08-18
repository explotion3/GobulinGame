#pragma once

#include "CoreMinimal.h"
#include "Combat/CombatantEndpoint.h"
#include "Enemy/EnemyNavigationTypes.h"
#include "Enemy/EnemyStateTypes.h"
#include "GameFramework/Character.h"
#include "UObject/PrimaryAssetId.h"
#include "GobulinEnemyActor.generated.h"

class UCapsuleComponent;
class UGobulinEnemyArchetype;
class UGobulinEnemyPresentationComponent;
class USceneComponent;

/** Thin Actor representation for one enemy. Gameplay state is owned by UGobulinEnemySubsystem. */
UCLASS(Blueprintable, NotPlaceable)
class GOBULINGAME_API AGobulinEnemyActor : public ACharacter, public ICombatantEndpoint
{
	GENERATED_BODY()

public:
	AGobulinEnemyActor();

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
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

	void InitializeEnemy(FCombatantHandle InHandle, const UGobulinEnemyArchetype& Archetype);
	void ApplyEnemyState(EEnemyState NewState);
	EEnemyMoveStatus RequestMoveToTarget(AActor* TargetActor, const FEnemyMoveIntent& Intent);
	void StopEnemyMovement();
	void SetEnemyCollisionEnabled(bool bEnabled);
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

};
