#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GobulinCameraFeedbackComponent.generated.h"

class UCameraComponent;

/** Adds local-player camera motion without replacing the authored camera transform. */
UCLASS(ClassGroup = (Camera), meta = (BlueprintSpawnableComponent))
class GOBULINGAME_API UGobulinCameraFeedbackComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGobulinCameraFeedbackComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Camera Feedback")
	void SetCamera(UCameraComponent* InCamera);

	/** Enables the movement-driven vertical camera bob. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Feedback|Movement")
	bool bEnableMovementBob = true;

	/** Number of complete bob cycles per second at full movement speed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Feedback|Movement", meta = (ClampMin = "0.0"))
	float MovementBobFrequency = 1.8f;

	/** Maximum vertical bob distance in Unreal units. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Feedback|Movement", meta = (ClampMin = "0.0"))
	float MovementBobAmplitude = 2.5f;

	/** Movement speed below which the bob starts fading out. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Feedback|Movement", meta = (ClampMin = "0.0"))
	float MovementBobSpeedThreshold = 10.0f;

	/** How quickly the bob fades in and out when movement state changes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Feedback|Movement", meta = (ClampMin = "0.0"))
	float MovementBobBlendSpeed = 8.0f;

	/** Enables the small roll applied while strafing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Feedback|Movement")
	bool bEnableStrafeRoll = true;

	/** Maximum camera roll in degrees while strafing at full speed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Feedback|Movement", meta = (ClampMin = "0.0"))
	float StrafeRollAngle = 1.0f;

	/** How quickly the strafe roll returns to its target. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Feedback|Movement", meta = (ClampMin = "0.0"))
	float StrafeRollInterpSpeed = 8.0f;

protected:
	UPROPERTY(Transient)
	TObjectPtr<UCameraComponent> Camera;

	float MovementBobPhase = 0.0f;
	float MovementBobWeight = 0.0f;
	float CurrentStrafeRoll = 0.0f;

	bool IsLocalPlayerFeedback() const;
	void ApplyCameraFeedback(float VerticalOffset, float RollOffset);
	void ClearCameraFeedback();
};
