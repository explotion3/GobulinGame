#include "Combat/GobulinHitStopSubsystem.h"

#include "Containers/Ticker.h"
#include "Kismet/GameplayStatics.h"

void UGobulinHitStopSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UGobulinHitStopSubsystem::Deinitialize()
{
	if (RealTimeTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(RealTimeTickerHandle);
		RealTimeTickerHandle.Reset();
	}

	RestoreTimeDilation();
	Super::Deinitialize();
}

void UGobulinHitStopSubsystem::RequestHitStop(float Duration, float TimeDilation)
{
	if (!GetWorld())
	{
		return;
	}

	const float SafeDuration = FMath::Max(0.0f, Duration);
	if (SafeDuration <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const float SafeTimeDilation = FMath::Clamp(TimeDilation, 0.01f, 1.0f);
	if (!bHitStopActive)
	{
		PreviousTimeDilation = UGameplayStatics::GetGlobalTimeDilation(this);
		AppliedTimeDilation = SafeTimeDilation;
	}
	else
	{
		AppliedTimeDilation = FMath::Min(AppliedTimeDilation, SafeTimeDilation);
	}

	RemainingDuration = FMath::Max(RemainingDuration, SafeDuration);
	bHitStopActive = true;
	UGameplayStatics::SetGlobalTimeDilation(this, AppliedTimeDilation);

	if (!RealTimeTickerHandle.IsValid())
	{
		const TWeakObjectPtr<UGobulinHitStopSubsystem> WeakThis(this);
		TUniqueFunction<bool(float)> TickerCallback =
			[WeakThis](float DeltaTime)
			{
				UGobulinHitStopSubsystem* HitStop = WeakThis.Get();
				return HitStop && HitStop->TickRestore(DeltaTime);
			};
		RealTimeTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
			TEXT("GobulinHitStop"),
			0.0f,
			MoveTemp(TickerCallback));
	}
}

bool UGobulinHitStopSubsystem::TickRestore(float DeltaTime)
{
	if (!bHitStopActive)
	{
		return false;
	}

	RemainingDuration -= FMath::Max(0.0f, DeltaTime);
	if (RemainingDuration > 0.0f)
	{
		return true;
	}

	RestoreTimeDilation();
	RealTimeTickerHandle.Reset();
	return false;
}

void UGobulinHitStopSubsystem::RestoreTimeDilation()
{
	if (!bHitStopActive)
	{
		return;
	}

	if (GetWorld())
	{
		const float CurrentTimeDilation = UGameplayStatics::GetGlobalTimeDilation(this);
		if (FMath::IsNearlyEqual(CurrentTimeDilation, AppliedTimeDilation, 0.001f))
		{
			UGameplayStatics::SetGlobalTimeDilation(this, PreviousTimeDilation);
		}
	}

	RemainingDuration = 0.0f;
	bHitStopActive = false;
	AppliedTimeDilation = 1.0f;
	PreviousTimeDilation = 1.0f;
}
