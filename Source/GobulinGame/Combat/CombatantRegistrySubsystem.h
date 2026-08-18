#pragma once

#include "CoreMinimal.h"
#include "Combat/CombatantHandle.h"
#include "Combat/CombatantSnapshot.h"
#include "Subsystems/WorldSubsystem.h"
#include "UObject/ObjectKey.h"
#include "CombatantRegistrySubsystem.generated.h"

class AActor;

/**
 * Game-thread registry that maps backend-neutral combatant handles to Actor endpoints.
 * A future Mass adapter can add an entity mapping without changing public protocol types.
 */
UCLASS()
class GOBULINGAME_API UCombatantRegistrySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	FCombatantHandle RegisterActor(AActor* Actor, uint8 TeamId = 0);
	bool UnregisterActor(const AActor* Actor);
	bool UnregisterHandle(FCombatantHandle Handle);
	bool SetCombatantActive(FCombatantHandle Handle, bool bActive);
	bool SetCombatantTeam(FCombatantHandle Handle, uint8 TeamId);

	FCombatantHandle FindHandleForActor(const AActor* Actor) const;
	AActor* ResolveActor(FCombatantHandle Handle) const;
	bool IsHandleValid(FCombatantHandle Handle) const;
	bool GetCombatantSnapshot(FCombatantHandle Handle, FCombatantSnapshot& OutSnapshot) const;
	void GetCombatantSnapshots(TArray<FCombatantSnapshot>& OutSnapshots) const;

	virtual void Deinitialize() override;

private:
	struct FRegistrySlot
	{
		TWeakObjectPtr<AActor> Actor;
		int32 Generation = 0;
		uint8 TeamId = 0;
		bool bOccupied = false;
		bool bActive = false;
	};

	TArray<FRegistrySlot> Slots;
	TArray<int32> FreeSlotIndices;
	TMap<FObjectKey, FCombatantHandle> ActorHandles;
};
