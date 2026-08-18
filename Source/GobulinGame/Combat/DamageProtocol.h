#pragma once

#include "CoreMinimal.h"
#include "Combat/CombatantHandle.h"
#include "GameplayTagContainer.h"
#include "DamageProtocol.generated.h"

UENUM(BlueprintType, meta = (Bitflags))
enum class ECombatDamageFlags : uint8
{
	None = 0 UMETA(Hidden),
	Critical = 1 << 0,
	IgnoreArmor = 1 << 1,
	DamageOverTime = 1 << 2,
	CannotKill = 1 << 3
};
ENUM_CLASS_FLAGS(ECombatDamageFlags);

UENUM(BlueprintType, meta = (ScriptName = "CombatDamageOutcome"))
enum class ECombatDamageResult : uint8
{
	InvalidRequest,
	InvalidTarget,
	Applied,
	Blocked,
	Immune,
	AlreadyDead,
	Duplicate
};

/** Backend-neutral request to apply damage to one combatant. */
USTRUCT(BlueprintType)
struct GOBULINGAME_API FCombatDamageRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Damage")
	FCombatCommandId CommandId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Damage")
	FCombatantHandle Source;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Damage")
	FCombatantHandle Target;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Damage", meta = (ClampMin = "0.0"))
	float BaseAmount = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Damage", meta = (Categories = "Combat.Attack"))
	FGameplayTag AttackTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Damage", meta = (Categories = "Combat.Damage"))
	FGameplayTag DamageType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Damage", meta = (Bitmask, BitmaskEnum = "/Script/GobulinGame.ECombatDamageFlags"))
	int32 Flags = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Damage")
	FVector HitLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Damage")
	FVector HitNormal = FVector::UpVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Damage")
	FVector Impulse = FVector::ZeroVector;

	bool IsValid() const
	{
		return Target.IsSet()
			&& FMath::IsFinite(BaseAmount)
			&& BaseAmount > 0.0f
			&& !HitLocation.ContainsNaN()
			&& !HitNormal.ContainsNaN()
			&& !Impulse.ContainsNaN();
	}

	bool HasFlag(ECombatDamageFlags Flag) const
	{
		return EnumHasAnyFlags(static_cast<ECombatDamageFlags>(Flags), Flag);
	}
};

/** Authoritative outcome produced after a damage request has been resolved. */
USTRUCT(BlueprintType)
struct GOBULINGAME_API FCombatDamageResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Damage")
	FCombatCommandId CommandId;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Damage")
	FCombatantHandle Source;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Damage")
	FCombatantHandle Target;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Damage")
	ECombatDamageResult Result = ECombatDamageResult::InvalidRequest;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Damage")
	float RequestedAmount = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Damage")
	float MitigatedAmount = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Damage")
	float AppliedAmount = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Damage")
	float RemainingHealth = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Damage", meta = (Categories = "Combat.Reaction"))
	FGameplayTag ReactionTag;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Damage")
	bool bKilled = false;

	bool DidApplyDamage() const
	{
		return Result == ECombatDamageResult::Applied && AppliedAmount > KINDA_SMALL_NUMBER;
	}
};

/** Confirmed damage fact emitted after a request has finished processing. */
USTRUCT(BlueprintType)
struct GOBULINGAME_API FCombatDamageResolvedEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Events")
	FCombatDamageRequest Request;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Events")
	FCombatDamageResult Result;
};
