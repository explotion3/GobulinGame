#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GobulinSwordAnimationPreviewActor.generated.h"

class ULevelSequence;
class UStaticMeshComponent;
class UGobulinSwordDefinition;
class UGobulinWeaponViewComponent;

/**
 * Editor-only-in-practice actor used to author a sword attack in Sequencer.
 *
 * The actor is intentionally separate from the player. Its root is the same
 * transform pivot used by the runtime weapon view, while the child visual
 * keeps its designer-authored handle alignment.
 */
UCLASS()
class GOBULINGAME_API AGobulinSwordAnimationPreviewActor : public AActor
{
	GENERATED_BODY()

public:
	AGobulinSwordAnimationPreviewActor();

	virtual void OnConstruction(const FTransform& Transform) override;

	/** Samples the assigned Level Sequence and replaces the sword attack curves. */
	UFUNCTION(CallInEditor, Category = "Sword Animation|Bake")
	void BakeAttackSequence();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sword Animation")
	TObjectPtr<UGobulinSwordDefinition> SwordDefinition;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sword Animation|Bake")
	TObjectPtr<ULevelSequence> AttackSequence;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sword Animation|Bake", meta = (ClampMin = "1", UIMin = "1"))
	int32 BakeSampleRate = 60;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sword Animation")
	TObjectPtr<UGobulinWeaponViewComponent> SwordPivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sword Animation")
	TObjectPtr<UStaticMeshComponent> SwordVisual;
};
