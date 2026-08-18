#pragma once

#include "CoreMinimal.h"
#include "Combat/CombatantHandle.h"
#include "CombatantSnapshot.generated.h"

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
	bool bActive = false;

	bool IsValid() const
	{
		return Handle.IsSet() && !Location.ContainsNaN();
	}
};
