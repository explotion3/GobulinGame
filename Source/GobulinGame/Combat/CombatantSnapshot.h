#pragma once

#include "CoreMinimal.h"
#include "Combat/CombatantHandle.h"
#include "CombatantSnapshot.generated.h"

/** 战斗单位参与接触判断的后端中立直立胶囊；不引用碰撞组件。 */
USTRUCT(BlueprintType)
struct GOBULINGAME_API FCombatantBodyShape
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Snapshot", meta = (Units = "cm"))
	float CapsuleRadius = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Snapshot", meta = (Units = "cm"))
	float CapsuleHalfHeight = 1.0f;

	bool IsValid() const
	{
		return FMath::IsFinite(CapsuleRadius)
			&& CapsuleRadius > 0.0f
			&& FMath::IsFinite(CapsuleHalfHeight)
			&& CapsuleHalfHeight >= CapsuleRadius;
	}
};

/** 战斗单位在某一帧的后端中立查询快照；不携带 Actor 或 Mass Entity 指针。 */
USTRUCT(BlueprintType)
struct GOBULINGAME_API FCombatantSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Snapshot")
	FCombatantHandle Handle;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Snapshot")
	uint8 TeamId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Snapshot")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Snapshot")
	FCombatantBodyShape BodyShape;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Snapshot")
	bool bActive = false;

	bool IsValid() const
	{
		return Handle.IsSet() && !Location.ContainsNaN() && BodyShape.IsValid();
	}
};
