#include "Combat/CombatantRegistrySubsystem.h"

#include "Combat/CombatantEndpoint.h"
#include "GameFramework/Actor.h"
#include "UObject/ObjectKey.h"

FCombatantHandle UCombatantRegistrySubsystem::RegisterActor(AActor* Actor, uint8 TeamId)
{
	check(IsInGameThread());

	if (!IsValid(Actor) || !Actor->GetClass()->ImplementsInterface(UCombatantEndpoint::StaticClass()))
	{
		return FCombatantHandle();
	}

	const FObjectKey ActorKey(Actor);
	if (const FCombatantHandle* ExistingHandle = ActorHandles.Find(ActorKey))
	{
		if (IsHandleValid(*ExistingHandle))
		{
			FRegistrySlot& ExistingSlot = Slots[ExistingHandle->GetIndex()];
			ExistingSlot.TeamId = TeamId;
			ExistingSlot.bActive = true;
			return *ExistingHandle;
		}
		ActorHandles.Remove(ActorKey);
	}

	int32 SlotIndex = INDEX_NONE;
	if (FreeSlotIndices.Num() > 0)
	{
		SlotIndex = FreeSlotIndices.Pop(EAllowShrinking::No);
	}
	else
	{
		SlotIndex = Slots.AddDefaulted();
	}

	FRegistrySlot& Slot = Slots[SlotIndex];
	Slot.Generation = Slot.Generation >= MAX_int32 ? 1 : FMath::Max(1, Slot.Generation + 1);
	Slot.Actor = Actor;
	Slot.TeamId = TeamId;
	Slot.bOccupied = true;
	Slot.bActive = true;

	const FCombatantHandle Handle(SlotIndex, Slot.Generation);
	ActorHandles.Add(ActorKey, Handle);
	return Handle;
}

bool UCombatantRegistrySubsystem::UnregisterActor(const AActor* Actor)
{
	check(IsInGameThread());

	if (!Actor)
	{
		return false;
	}

	const FObjectKey ActorKey(Actor);
	const FCombatantHandle* Handle = ActorHandles.Find(ActorKey);
	return Handle ? UnregisterHandle(*Handle) : false;
}

bool UCombatantRegistrySubsystem::UnregisterHandle(FCombatantHandle Handle)
{
	check(IsInGameThread());

	if (!Handle.IsSet() || !Slots.IsValidIndex(Handle.GetIndex()))
	{
		return false;
	}

	FRegistrySlot& Slot = Slots[Handle.GetIndex()];
	if (!Slot.bOccupied || Slot.Generation != Handle.GetGeneration())
	{
		return false;
	}

	if (AActor* Actor = Slot.Actor.Get())
	{
		ActorHandles.Remove(FObjectKey(Actor));
	}

	Slot.Actor.Reset();
	Slot.TeamId = 0;
	Slot.bOccupied = false;
	Slot.bActive = false;
	FreeSlotIndices.Add(Handle.GetIndex());
	return true;
}

bool UCombatantRegistrySubsystem::SetCombatantActive(FCombatantHandle Handle, bool bActive)
{
	check(IsInGameThread());

	if (!IsHandleValid(Handle))
	{
		return false;
	}

	Slots[Handle.GetIndex()].bActive = bActive;
	return true;
}

bool UCombatantRegistrySubsystem::SetCombatantTeam(FCombatantHandle Handle, uint8 TeamId)
{
	check(IsInGameThread());

	if (!IsHandleValid(Handle))
	{
		return false;
	}

	Slots[Handle.GetIndex()].TeamId = TeamId;
	return true;
}

FCombatantHandle UCombatantRegistrySubsystem::FindHandleForActor(const AActor* Actor) const
{
	check(IsInGameThread());

	if (!Actor)
	{
		return FCombatantHandle();
	}

	if (const FCombatantHandle* Handle = ActorHandles.Find(FObjectKey(Actor)))
	{
		return IsHandleValid(*Handle) ? *Handle : FCombatantHandle();
	}

	return FCombatantHandle();
}

AActor* UCombatantRegistrySubsystem::ResolveActor(FCombatantHandle Handle) const
{
	check(IsInGameThread());
	return IsHandleValid(Handle) ? Slots[Handle.GetIndex()].Actor.Get() : nullptr;
}

bool UCombatantRegistrySubsystem::IsHandleValid(FCombatantHandle Handle) const
{
	if (!Handle.IsSet() || !Slots.IsValidIndex(Handle.GetIndex()))
	{
		return false;
	}

	const FRegistrySlot& Slot = Slots[Handle.GetIndex()];
	return Slot.bOccupied
		&& Slot.Generation == Handle.GetGeneration()
		&& Slot.Actor.IsValid();
}

bool UCombatantRegistrySubsystem::GetCombatantSnapshot(
	FCombatantHandle Handle,
	FCombatantSnapshot& OutSnapshot) const
{
	check(IsInGameThread());

	if (!IsHandleValid(Handle))
	{
		return false;
	}

	const FRegistrySlot& Slot = Slots[Handle.GetIndex()];
	const AActor* Actor = Slot.Actor.Get();
	if (!Actor)
	{
		return false;
	}

	OutSnapshot.Handle = Handle;
	OutSnapshot.TeamId = Slot.TeamId;
	OutSnapshot.Location = Actor->GetActorLocation();
	OutSnapshot.bActive = Slot.bActive;
	return true;
}

void UCombatantRegistrySubsystem::GetCombatantSnapshots(
	TArray<FCombatantSnapshot>& OutSnapshots) const
{
	check(IsInGameThread());

	OutSnapshots.Reset();
	OutSnapshots.Reserve(Slots.Num() - FreeSlotIndices.Num());
	for (int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex)
	{
		const FRegistrySlot& Slot = Slots[SlotIndex];
		const AActor* Actor = Slot.Actor.Get();
		if (!Slot.bOccupied || !Actor)
		{
			continue;
		}

		FCombatantSnapshot& Snapshot = OutSnapshots.AddDefaulted_GetRef();
		Snapshot.Handle = FCombatantHandle(SlotIndex, Slot.Generation);
		Snapshot.TeamId = Slot.TeamId;
		Snapshot.Location = Actor->GetActorLocation();
		Snapshot.bActive = Slot.bActive;
	}
}

void UCombatantRegistrySubsystem::Deinitialize()
{
	Slots.Reset();
	FreeSlotIndices.Reset();
	ActorHandles.Reset();
	Super::Deinitialize();
}
