#pragma once

#include "CoreMinimal.h"
#include "CombatantHandle.generated.h"

class UPackageMap;

/**
 * World-scoped logical identity for every combat participant.
 *
 * The handle deliberately carries no Actor or Mass representation. Index identifies a
 * registry slot while Generation prevents a stale handle from resolving after slot reuse.
 */
USTRUCT(BlueprintType)
struct GOBULINGAME_API FCombatantHandle
{
	GENERATED_BODY()

public:
	FCombatantHandle() = default;
	FCombatantHandle(int32 InIndex, int32 InGeneration)
		: Index(InIndex)
		, Generation(InGeneration)
	{
	}

	bool IsSet() const
	{
		return Index != INDEX_NONE && Generation > 0;
	}

	void Reset()
	{
		Index = INDEX_NONE;
		Generation = 0;
	}

	int32 GetIndex() const { return Index; }
	int32 GetGeneration() const { return Generation; }

	FString ToString() const
	{
		return IsSet()
			? FString::Printf(TEXT("%d:%d"), Index, Generation)
			: TEXT("Invalid");
	}

	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
	{
		(void)Map;

		uint32 PackedIndex = IsSet() ? static_cast<uint32>(Index + 1) : 0;
		uint32 PackedGeneration = Generation > 0 ? static_cast<uint32>(Generation) : 0;
		Ar.SerializeIntPacked(PackedIndex);
		Ar.SerializeIntPacked(PackedGeneration);

		if (Ar.IsLoading())
		{
			Index = PackedIndex > 0 ? static_cast<int32>(PackedIndex - 1) : INDEX_NONE;
			Generation = static_cast<int32>(PackedGeneration);
		}

		bOutSuccess = PackedIndex == 0 || PackedGeneration > 0;
		return true;
	}

	friend bool operator==(const FCombatantHandle& Left, const FCombatantHandle& Right)
	{
		return Left.Index == Right.Index && Left.Generation == Right.Generation;
	}

	friend bool operator!=(const FCombatantHandle& Left, const FCombatantHandle& Right)
	{
		return !(Left == Right);
	}

	friend uint32 GetTypeHash(const FCombatantHandle& Handle)
	{
		return HashCombine(::GetTypeHash(Handle.Index), ::GetTypeHash(Handle.Generation));
	}

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	int32 Index = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	int32 Generation = 0;
};

template<>
struct TStructOpsTypeTraits<FCombatantHandle> : public TStructOpsTypeTraitsBase2<FCombatantHandle>
{
	enum
	{
		WithNetSerializer = true,
		WithIdenticalViaEquality = true
	};
};

/** Identifier shared by a command and every event caused by that command. */
USTRUCT(BlueprintType)
struct GOBULINGAME_API FCombatCommandId
{
	GENERATED_BODY()

public:
	FCombatCommandId() = default;
	explicit FCombatCommandId(int64 InValue)
		: Value(InValue)
	{
	}

	bool IsSet() const { return Value > 0; }
	int64 GetValue() const { return Value; }

	FString ToString() const
	{
		return IsSet() ? LexToString(Value) : TEXT("Invalid");
	}

	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
	{
		(void)Map;

		uint64 PackedValue = Value > 0 ? static_cast<uint64>(Value) : 0;
		Ar.SerializeIntPacked64(PackedValue);
		if (Ar.IsLoading())
		{
			Value = PackedValue <= static_cast<uint64>(MAX_int64)
				? static_cast<int64>(PackedValue)
				: 0;
		}

		bOutSuccess = PackedValue <= static_cast<uint64>(MAX_int64);
		return true;
	}

	friend bool operator==(const FCombatCommandId& Left, const FCombatCommandId& Right)
	{
		return Left.Value == Right.Value;
	}

	friend bool operator!=(const FCombatCommandId& Left, const FCombatCommandId& Right)
	{
		return !(Left == Right);
	}

	friend uint32 GetTypeHash(const FCombatCommandId& CommandId)
	{
		return ::GetTypeHash(CommandId.Value);
	}

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	int64 Value = 0;
};

template<>
struct TStructOpsTypeTraits<FCombatCommandId> : public TStructOpsTypeTraitsBase2<FCombatCommandId>
{
	enum
	{
		WithNetSerializer = true,
		WithIdenticalViaEquality = true
	};
};
