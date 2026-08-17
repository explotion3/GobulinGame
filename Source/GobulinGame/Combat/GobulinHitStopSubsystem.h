#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Subsystems/WorldSubsystem.h"
#include "GobulinHitStopSubsystem.generated.h"

/** Coordinates short world-level hit stops and restores time using real engine time. */
UCLASS()
class GOBULINGAME_API UGobulinHitStopSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Combat|Hit Stop")
	void RequestHitStop(float Duration, float TimeDilation = 0.05f);

	UFUNCTION(BlueprintPure, Category = "Combat|Hit Stop")
	bool IsHitStopActive() const { return bHitStopActive; }

protected:
	bool TickRestore(float DeltaTime);
	void RestoreTimeDilation();

	FTSTicker::FDelegateHandle RealTimeTickerHandle;
	float RemainingDuration = 0.0f;
	float PreviousTimeDilation = 1.0f;
	float AppliedTimeDilation = 1.0f;
	bool bHitStopActive = false;
};
