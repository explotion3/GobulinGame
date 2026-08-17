#pragma once

#include "CoreMinimal.h"
#include "Combat/GobulinSwordCombatComponent.h"
#include "Components/ActorComponent.h"
#include "GobulinSwordFeedbackComponent.generated.h"

/** Presents sword combat events without owning attack state or hit detection. */
UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class GOBULINGAME_API UGobulinSwordFeedbackComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGobulinSwordFeedbackComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category = "Sword Feedback")
	void SetSwordCombat(UGobulinSwordCombatComponent* InSwordCombat);

protected:
	UPROPERTY(Transient)
	TObjectPtr<UGobulinSwordCombatComponent> SwordCombat;

	bool bHitCameraShakePlayed = false;
	bool bHitStopPlayed = false;

	UFUNCTION()
	void HandleAttackStateChanged(EGobulinSwordAttackState PreviousState, EGobulinSwordAttackState NewState);

	UFUNCTION()
	void HandleSwingTriggered();

	UFUNCTION()
	void HandleHitConfirmed(AActor* HitActor, FVector HitLocation, FVector HitNormal, float AppliedDamage, bool bKilled);

	void BindToSwordCombat();
	void UnbindFromSwordCombat();
	bool IsLocalPlayerFeedback() const;
};
