#pragma once

#include "CoreMinimal.h"
#include "Combat/CombatantHandle.h"
#include "EnemyNavigationTypes.generated.h"

UENUM(BlueprintType)
enum class EEnemyTargetChangeReason : uint8
{
	Acquired,
	TargetInactive,
	OutOfRange,
	NavigationFailed
};

UENUM(BlueprintType)
enum class EEnemyMoveStatus : uint8
{
	Idle,
	Moving,
	Reached,
	Blocked,
	Failed
};

/** 单个敌人的目标记忆；只保存稳定句柄和观测数据。 */
USTRUCT(BlueprintType)
struct GOBULINGAME_API FEnemyTargetData
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Target")
	FCombatantHandle Handle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Target")
	FVector LastKnownLocation = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Target")
	float LastUpdateTime = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Target")
	int32 Revision = 0;

	bool IsSet() const { return Handle.IsSet(); }

	void Assign(FCombatantHandle NewHandle, const FVector& Location, float WorldTime)
	{
		Handle = NewHandle;
		LastKnownLocation = Location;
		LastUpdateTime = WorldTime;
		++Revision;
	}

	void Refresh(const FVector& Location, float WorldTime)
	{
		LastKnownLocation = Location;
		LastUpdateTime = WorldTime;
	}

	void Clear(float WorldTime)
	{
		Handle.Reset();
		LastKnownLocation = FVector::ZeroVector;
		LastUpdateTime = WorldTime;
		++Revision;
	}
};

/** 后端无关的移动意图；Actor Controller 与未来 Mass 移动处理器消费同一语义。 */
USTRUCT(BlueprintType)
struct GOBULINGAME_API FEnemyMoveIntent
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Movement")
	FCombatantHandle Target;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Movement")
	FVector Destination = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Movement")
	float DesiredSpeed = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Movement")
	float AcceptanceRadius = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Movement")
	float RequestedTime = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Movement")
	int32 IntentSequence = 0;

	bool IsValid() const
	{
		return Target.IsSet()
			&& !Destination.ContainsNaN()
			&& FMath::IsFinite(DesiredSpeed)
			&& DesiredSpeed >= 0.0f
			&& FMath::IsFinite(AcceptanceRadius)
			&& AcceptanceRadius >= 0.0f
			&& FMath::IsFinite(RequestedTime)
			&& IntentSequence > 0;
	}
};

/** 每个敌人的可变移动状态；不保存路径对象、Controller 或 Actor。 */
USTRUCT(BlueprintType)
struct GOBULINGAME_API FEnemyMovementData
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Movement")
	FEnemyMoveIntent Intent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Movement")
	EEnemyMoveStatus Status = EEnemyMoveStatus::Idle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Movement")
	float StatusChangeTime = 0.0f;

	/** 单调递增，不因停止或清空当前意图而复位。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Movement")
	int32 LastIntentSequence = 0;
};
