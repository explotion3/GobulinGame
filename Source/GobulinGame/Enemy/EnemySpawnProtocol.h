#pragma once

#include "CoreMinimal.h"
#include "Combat/CombatantHandle.h"
#include "UObject/PrimaryAssetId.h"
#include "EnemySpawnProtocol.generated.h"

UENUM(BlueprintType)
enum class EEnemySpawnResult : uint8
{
	Success,
	InvalidRequest,
	Duplicate,
	DefinitionUnavailable,
	BackendUnavailable,
	SpawnBlocked
};

/** Backend-neutral request for one enemy instance. Batch APIs consume arrays of this value. */
USTRUCT(BlueprintType)
struct GOBULINGAME_API FEnemySpawnRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Spawn")
	FCombatCommandId CommandId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Spawn")
	FPrimaryAssetId EnemyDefinitionId;

	/** 后端无关的地面锚点；Actor 后端会按胶囊半高转换为实际 Actor 原点。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Spawn")
	FTransform SpawnTransform = FTransform::Identity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Spawn")
	FCombatantHandle Owner;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Spawn")
	uint8 TeamId = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Spawn", meta = (ClampMin = "1"))
	int32 EnemyLevel = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Spawn", meta = (ClampMin = "0.01"))
	float PowerScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Spawn")
	int32 RandomSeed = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Spawn")
	int32 SpawnGroupId = INDEX_NONE;

	bool IsValid() const
	{
		return EnemyDefinitionId.IsValid()
			&& !SpawnTransform.ContainsNaN()
			&& EnemyLevel > 0
			&& FMath::IsFinite(PowerScale)
			&& PowerScale > 0.0f;
	}
};

USTRUCT(BlueprintType)
struct GOBULINGAME_API FEnemySpawnResultData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Enemy|Spawn")
	FCombatCommandId CommandId;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy|Spawn")
	EEnemySpawnResult Result = EEnemySpawnResult::InvalidRequest;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy|Spawn")
	FCombatantHandle Enemy;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy|Spawn")
	FPrimaryAssetId EnemyDefinitionId;
};
